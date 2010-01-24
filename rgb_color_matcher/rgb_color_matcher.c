#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <math.h> //for square root function

#define LED_RED_AUTO PD7 // pin 17 (atmega)
#define LED_GREEN_AUTO PD6
#define LED_BLUE_AUTO PD5
#define LED_RED_USER PD4
#define LED_GREEN_USER PD3
#define LED_BLUE_USER PD2
#define OUTPUT_MASK_AUTO (1 << PD7) | (1 << PD6) | (1 << PD5)
#define OUTPUT_MASK_USER (1 << PD4) | (1 << PD3) | (1 << PD2)
#define SPEAKER_PIN PB3
#define RED_POT PC0 //adc0
#define GREEN_POT PC1
#define BLUE_POT PC2
#define INPUT_MASK ((1 << PC0) | (1 << PC1) | (1 << PC2))
#define PWM_MIN 0
#define PWM_MAX 225

//define the processor speed (if it's not already been defined by the compiler)
#ifndef F_CPU
	#define F_CPU 8000000UL
#endif
/**
 * TODO add switch for adjusting the difficulty (speaker on/off).
 */
uint8_t mUserLedLevels[3] = {0,0,0};
uint8_t mAutoLedLevels[3] = {150, 0, 30};
uint8_t mBuffer[6];
uint8_t i;
uint8_t kDelta = 50;
uint8_t mAdcChannel[3] = {RED_POT, GREEN_POT, BLUE_POT}; //is this correct?
uint8_t mSpeakerOn = 1;

uint8_t readAdc(uint8_t pChannel)
{
	//clear the previous channel selection
	ADMUX &= 0b11111000;

	//set the new channel
	ADMUX |= pChannel;

	// Start ADC conversion
	ADCSRA |= (1<<ADSC);

	while(!(ADCSRA & (1 << ADIF)));

	return(ADCH);
}

void initTimers()
{
	//enable timer2 overflow interrupt
	//TIMSK = (1 << TOIE2);

	//enable timer/counter0 overflow interrupt
	TIMSK |= (1 << TOIE0);

	//start timer2, 256 prescale
	TCCR2 = (1 << CS22) | (1 << CS21);

	//start timer0, no prescale
	TCCR0 = (1 << CS00);

	//set ctc mode for timer2 PWM
	TCCR2 |= (1 << WGM21);

	//set toggle logical level on each compare match
	//this allows waveform generation in CTC mode
	TCCR2 |= (1 << COM20);

	//gives frequency of ((8MHz)/256)/255) = 122.55 Hz
	OCR2 = 255;

	//enable interrupts
	sei();
}

void initAdc()
{
	// Enable ADC
	ADCSRA = (1 << ADEN);

	// Select divider factor 8, so we get 1 MHz/8 = 125 kHz ADC clock
	ADCSRA |= (1<<ADPS1) | (1<<ADPS0);

	// we'll select the channel each time we read the ADC

	// Use Vcc as voltage reference
	ADMUX |= (1 << REFS0);

	//we only need 8-bit precision.  Left adjust the ADC result
	//so that we can read the ADCH register and be done with it.
	ADMUX |= (1 << ADLAR);
}

void setAutoLeds()
{
	for(i = 0; i < 3; i++)
	{
		mAutoLedLevels[i] = (uint8_t)(rand()/112);
	}
}

//use a variable in EEPROM
void initRand()
{
	uint8_t vSeed = eeprom_read_word(0); // load last stored seed
	srand(++vSeed); // increment and use value as seed
	eeprom_write_word(0, vSeed); //store the new seed for next time
}

void flashAndReset()
{
	cli();
	for(i=0;i<5;i++)
	{
		if(mSpeakerOn)
		{
			OCR2 = i * 50;
		}
		PORTD |= OUTPUT_MASK_AUTO;
		_delay_ms(100);
		PORTD &=~ OUTPUT_MASK_AUTO;
		_delay_ms(100);
	}
	setAutoLeds();
	sei();
}

//compare the RGB vectors for the two RGB LEDs
//if they're within kDelta then flash and reset
//to a new value
void compareValues()
{
	uint16_t vDistance = 0;
	for(i=0;i<3;i++)
	{
		vDistance += pow((mUserLedLevels[i] - mAutoLedLevels[i]),2);
	}
	vDistance = sqrt(vDistance);

	if(mSpeakerOn)
	{
		OCR2 = vDistance/2;
	}

	if(vDistance <= kDelta)
	{
		flashAndReset();
	}
}

int main(void)
{
	DDRD |= OUTPUT_MASK_AUTO | OUTPUT_MASK_USER;//led set as output
	DDRC &=~ INPUT_MASK;//set potentiometer pin for input
	DDRB |= (1 << SPEAKER_PIN); //speaker output
	initTimers();
	initAdc();
	initRand();
	setAutoLeds();

	while (1)
	{
		for(i = 0; i < 3; i++)
		{
			mUserLedLevels[i] = readAdc(i);
			_delay_ms(5);
		}
		_delay_ms(10);
		compareValues();
	}
}

/*
 * Timer/Counter overflow interrupt (timer0). This is called each time
 * the counter overflows (255 counts/cycles).
 */
ISR(TIMER0_OVF_vect)
{
	//static variables maintain state from one call to the next
	static unsigned char sPortDmask = OUTPUT_MASK_AUTO | OUTPUT_MASK_USER;
	static unsigned char sCounter = 255;

	//set port pins straight away (no waiting for processing)
	PORTD = sPortDmask;

	//this counter will overflow back to 0 after reaching 255.
	//So we end up adjusting the LED states for every 256 overflows.
	if(++sCounter == 0)
	{
		for(i = 0; i < 3; i++)
		{
			mBuffer[i] = mUserLedLevels[i];
			mBuffer[i + 3] = mAutoLedLevels[i];
		}
		//set all pins to high
		sPortDmask = OUTPUT_MASK_AUTO | OUTPUT_MASK_USER;
	}
	//this loop is considered for every overflow interrupt.
	//this is the software PWM.
	if(mBuffer[0] == sCounter) sPortDmask &= ~(1 << LED_RED_USER);
	if(mBuffer[1] == sCounter) sPortDmask &= ~(1 << LED_GREEN_USER);
	if(mBuffer[2] == sCounter) sPortDmask &= ~(1 << LED_BLUE_USER);
	if(mBuffer[3] == sCounter) sPortDmask &= ~(1 << LED_RED_AUTO);
	if(mBuffer[4] == sCounter) sPortDmask &= ~(1 << LED_GREEN_AUTO);
	if(mBuffer[5] == sCounter) sPortDmask &= ~(1 << LED_BLUE_AUTO);
}
