/*
 * selectableColourLight.c
 *
 * Distributed under Creative Commons 3.0 -- Attib & Share Alike
 *
 * Based on the rgbLedNightLight project.  This code sets up a potentiometer as
 * input to the uC the value of which is used to select a colour.
 *
 *  Created on: Apr 11, 2010
 *      Author: Paul
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#ifndef F_CPU
    #define F_CPU 8000000UL
#endif

//Hardware definitions
#define RED_LED      PB2
#define GREEN_LED    PB1
#define BLUE_LED     PB0
#define ALL_LEDS    ((1 << RED_LED) | (1 << GREEN_LED) | (1 << BLUE_LED))

#define POT_PIN PB4

#define R_MAX 255
#define G_MAX 255
#define B_MAX 255

#define RED_INDEX   0
#define GREEN_INDEX 1
#define BLUE_INDEX  2

//set red to max (we start in the RedToYellow state)
unsigned char mRgbBuffer[] = {0,0,0};
volatile unsigned char mRgbValues[] = {0,0,0};
unsigned char mState;

void initAdc()
{
	// Enable ADC
	ADCSRA = (1 << ADEN);

	// Select divider factor 64, so we get 8 MHz/64 = 125 kHz ADC clock
	ADCSRA |= (1<<ADPS2) | (1<<ADPS1);

	// Use Vcc as voltage reference
	ADMUX |= (1 << REFS0);

	//we only need 8-bit precision.  Left adjust the ADC result
	//so that we can read the ADCH register and be done with it.
	ADMUX |= (1 << ADLAR);

	// To select ADC2 we set ADMUX register bits MUX3..0=0010
	ADMUX |= (1 << MUX1);
}

void init_timers()
{
    TIMSK = (1 << TOIE0); // enable overflow interrupt
    TCCR0B = (1 << CS00);  // start timer, no prescale

    //enable interrupts
    sei();
}

uint8_t readAdc()
{
	// Start ADC conversion
	ADCSRA |= (1<<ADSC);

	while(!(ADCSRA & (1 << ADIF)));

	return(ADCH);
}

/**
 * sets all 3 RGB values from the single input value.
 * This is based on the following transitions (as in the RGB Night light project)
 * 1. 255,  0,  0 ->
 * 2. 255,255,  0 ->
 * 3.   0,255,  0 ->
 * 4.   0,255,255 ->
 * 5.   0,  0,255 ->
 * 6. 255,  0,255 -> 1.
 *
 * The 8-bit input value has a max value of 255.  So, for the 6 transitions we have
 * 256/6 = 42.6 values to choose from.
 *
 * We scale the ADC input value as follows
 * ADC value < 43  = transition 1. (increase Green value)
 *           < 85  = 2. (decrease Red)
 *           < 117 = 3. (increase Bluse)
 *           < 159 = 4. (decrease Green)
 *           < 201 = 5. (increase Red)
 *           > 201 = 6. (decrease Blue)
 *
 * Then the value for the increment/decremented channel is as follows:
 *
 * ADC input * 255/n where 'n' is the transition number (1 - 6 above).
 * For decreasing values we use 255 - (ADC * 255/n) as the channel value.
 */
void setRgbLevels(uint8_t pValue)
{
	if(pValue < 43)
	{
		mRgbValues[RED_INDEX]   = 255;
		mRgbValues[GREEN_INDEX] = pValue * 255/1;
		mRgbValues[BLUE_INDEX]  = 0;

	}
	else if(pValue < 85)
	{
		mRgbValues[RED_INDEX]   = 255 - (pValue * 255/2);
		mRgbValues[GREEN_INDEX] = 255;
		mRgbValues[BLUE_INDEX]  = 0;
	}
	else if(pValue < 117)
	{
		mRgbValues[RED_INDEX]   = 0;
		mRgbValues[GREEN_INDEX] = 255;
		mRgbValues[BLUE_INDEX]  = pValue * 255/3;
	}
	else if(pValue < 159)
	{
		mRgbValues[RED_INDEX]   = 0;
		mRgbValues[GREEN_INDEX] = 255 - (pValue * 255/4);
		mRgbValues[BLUE_INDEX]  = 255;
	}
	else if(pValue < 201)
	{
		mRgbValues[RED_INDEX]   = pValue * 255/5;
		mRgbValues[GREEN_INDEX] = 0;
		mRgbValues[BLUE_INDEX]  = 255;
	}
	else
	{
		mRgbValues[RED_INDEX]   = 255;
		mRgbValues[GREEN_INDEX] = 0;
		mRgbValues[BLUE_INDEX]  = 255 - (pValue * 255/6);
	}
}

int main(void)
{
	//Set LED pins as output
	DDRB |= ALL_LEDS;

	//Set POT PIN as input
	DDRB &= ~(1 << POT_PIN);

	initAdc();
	init_timers();
	uint8_t mValue = 1;

	for(mValue = 0; mValue < 3; mValue++)
	{
		mRgbValues[RED_INDEX]   = 255;
		mRgbValues[GREEN_INDEX] = 0;
		mRgbValues[BLUE_INDEX]  = 0;
		_delay_ms(250);
		mRgbValues[RED_INDEX]   = 0;
		mRgbValues[GREEN_INDEX] = 255;
		mRgbValues[BLUE_INDEX]  = 0;
		_delay_ms(250);
		mRgbValues[RED_INDEX]   = 0;
		mRgbValues[GREEN_INDEX] = 0;
		mRgbValues[BLUE_INDEX]  = 255;
		_delay_ms(250);
	}
	for(;;)
	{
		mValue = readAdc();
		setRgbLevels(mValue);
		_delay_ms(150);
	}
	return 0;
}

/*
 * Timer/Counter overflow interrupt. This is called each time
 * the counter overflows (255 counts/cycles).
 */
ISR(TIM0_OVF_vect)
{
    //static variables maintain state from one call to the next
    static unsigned char sPortBmask = ALL_LEDS;
    static unsigned char sCounter = 255;

    //set port pins straight away (no waiting for processing)
    PORTB = sPortBmask | (1 << POT_PIN);

    //this counter will overflow back to 0 after reaching 255.
    //So we end up adjusting the LED states for every 256 interrupts/overflows.
    if(++sCounter == 0)
    {
    	mRgbBuffer[RED_INDEX] = mRgbValues[RED_INDEX];
    	mRgbBuffer[GREEN_INDEX] = mRgbValues[GREEN_INDEX];
    	mRgbBuffer[BLUE_INDEX] = mRgbValues[BLUE_INDEX];

        //set all pins to low (remember this is a common anode LED)
        sPortBmask &=~ ALL_LEDS;
        sPortBmask |= (1 << POT_PIN);
    }
    //this loop is considered for every overflow interrupt.
    //this is the software PWM.
    if(mRgbBuffer[RED_INDEX]   == sCounter) sPortBmask |= (1 << RED_LED);
    if(mRgbBuffer[GREEN_INDEX] == sCounter) sPortBmask |= (1 << GREEN_LED);
    if(mRgbBuffer[BLUE_INDEX]  == sCounter) sPortBmask |= (1 << BLUE_LED);
}
