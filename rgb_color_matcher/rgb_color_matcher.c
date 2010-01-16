#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <math.h>

#define LED_RED_AUTO PD7 // pin 17 (atmega)
#define LED_GREEN_AUTO PD6
#define LED_BLUE_AUTO PD5
#define LED_RED_USER PD4
#define LED_GREEN_USER PD3
#define LED_BLUE_USER PD2
#define OUTPUT_MASK_AUTO (1 << PD7) | (1 << PD6) | (1 << PD5)
#define OUTPUT_MASK_USER (1 << PD4) | (1 << PD3) | (1 << PD2)
#define RED_POT PC0 //adc0
#define GREEN_POT PC1
#define BLUE_POT PC2
#define INPUT_MASK ((1 << PC0) | (1 << PC1) | (1 << PC2))
#define PWM_MIN 0
#define PWM_MAX 225

#ifndef F_CPU
	#define F_CPU 8000000UL
#endif
/**
 * TODO read and write eeprom seeds for rand() so that the
 * values don't always start the same.
 * TODO calculate distance of user and auto colour vectors
 * TODO add flash for when user gets the colours to within d.
 * TODO add switch for adjusting the difficulty (change d).
 */
uint8_t mUserLedLevels[3] = {0,0,0};
uint8_t mAutoLedLevels[3] = {150, 0, 30};
uint8_t mBuffer[6];
uint8_t i;
uint8_t kDelta = 50;
uint8_t mAdcChannel[3] = {RED_POT, GREEN_POT, BLUE_POT}; //is this correct?

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
	//enable overflow interrupt
	TIMSK = (1 << TOIE2);

	//start timer, no prescale
	TCCR2 = (1 << CS20);

	//enable interrupts
	sei();
}

void initAdc()
{
	// Enable ADC
	ADCSRA = (1 << ADEN);

	// Select divider factor 8, so we get 1 MHz/8 = 125 kHz ADC clock
	ADCSRA |= (1<<ADPS1) | (1<<ADPS0);

	//we'll select the channel each time we read the ADC

	// Use Vcc as voltage reference
	ADMUX |= (1 << REFS0);

	//we only need 8-bit precision.  Left adjust the ADC result
	//so that we can read the ADCH register and be done with it.
	ADMUX |= (1 << ADLAR);


	//enable free running mode - comment out if using readAdc();
//	ADCSRA |= (1 << ADFR);

	//enable interrupts - comment this out if using single conversion i.e. readAdc()!
//	ADCSRA |= (1 << ADIE);

	//start conversions
//	ADCSRA |= (1 << ADSC);
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
	uint8_t vSeed = eeprom_read_word(0);
	srand(++vSeed);
	eeprom_write_word(0, vSeed);
}

void flashAndReset()
{
	cli();
	for(i=0;i<5;i++)
	{
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
	if(sqrt(vDistance) <= kDelta)
	{
		flashAndReset();
	}
}


int main(void)
{
	DDRD |= OUTPUT_MASK_AUTO | OUTPUT_MASK_USER;//led set as output
	DDRC &=~ INPUT_MASK;//set potentiometer pin for input
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
 * Timer/Counter overflow interrupt. This is called each time
 * the counter overflows (255 counts/cycles).
 */
ISR(TIMER2_OVF_vect)
{
	//static variables maintain state from one call to the next
	static unsigned char sPortBmask = OUTPUT_MASK_AUTO | OUTPUT_MASK_USER;
	static unsigned char sCounter = 255;

	//set port pins straight away (no waiting for processing)
	PORTD = sPortBmask;

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
		sPortBmask = OUTPUT_MASK_AUTO | OUTPUT_MASK_USER;
	}
	//this loop is considered for every overflow interrupt.
	//this is the software PWM.
	if(mBuffer[0] == sCounter) sPortBmask &= ~(1 << LED_RED_USER);
	if(mBuffer[1] == sCounter) sPortBmask &= ~(1 << LED_GREEN_USER);
	if(mBuffer[2] == sCounter) sPortBmask &= ~(1 << LED_BLUE_USER);
	if(mBuffer[3] == sCounter) sPortBmask &= ~(1 << LED_RED_AUTO);
	if(mBuffer[4] == sCounter) sPortBmask &= ~(1 << LED_GREEN_AUTO);
	if(mBuffer[5] == sCounter) sPortBmask &= ~(1 << LED_BLUE_AUTO);
}

/*
 * ADC interrupt
 */
//ISR(ADC_vect)
//{
//
//}
