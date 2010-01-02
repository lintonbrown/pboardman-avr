/*
 * FireflyJar.c
 *
 *  Created on: Jan 1, 2010
 *      Author: Paul
 */
#import <util/delay.h>
#import <avr/io.h>

#ifndef F_CPU
	#define F_CPU 1000000UL
#endif

//LED pin defs
#define LED1			PB0
#define LED2			PB1
#define LED3			PB2
#define LED4			PB3
#define LED5			PB4
#define N_LEDS			5
#define ALL_LEDS (1 << LED1) | (1 << LED2) | (1 << LED3) | (1 << LED4) | (1 << LED5)

//we'll use bit 7 to store directionality of the pulse
//1 for up and 0 for down
#define PULSE_DIRECTION	7

//Max value for brightness
#define MAX				200

//structure for storing data on each LED
struct ledData {
	unsigned char mBrightness;
	unsigned char mTime;
	unsigned char mPin;
};

//counter for use in updateLedState() and pulseLeds()
unsigned char i,j;

//set up some initial values
struct ledData led_data[] = {
		{0,180,LED1},
		{MAX,10,LED2},
		{10,200,LED3},
		{10,220,LED4},
		{200,0, LED5}
};

//TODO return a random time interval from 1 to 255
int getTime()
{
	return 50;
}

void pulseLeds()
{
	PORTB |= ALL_LEDS;
	for(i = 0; i < 255; i++)
	{
		for(j = 0; j < N_LEDS; j++)
		{
			if(led_data[j].mBrightness <= i)
			{
				PORTB &=~ (1 << led_data[j].mPin);
			}
		}
	}
	PORTB &=~ ALL_LEDS;
}

void updateLedState()
{
	for(i = 0; i < N_LEDS; i++)
	{
		switch(led_data[i].mBrightness)
		{
			case MAX://led is on
				if(--led_data[i].mTime == 0)
				{
					//decrement the brightness, this puts the LED in a
					//pulse state
					led_data[i].mBrightness--;

					//specify the "down" direction for pulsing
					led_data[i].mPin &=~ (1 << PULSE_DIRECTION);
				}
				break;
			case 0://led is off
				if(--led_data[i].mTime == 0)
				{
					//increment the brightness,this puts the LED in
					//a pulse state
					led_data[i].mBrightness++;

					//specify the "up" direction for pulsing
					led_data[i].mPin |= (1 << PULSE_DIRECTION);

					//set the ON time
					led_data[i].mTime = getTime();
				}
				break;
			default: //pulse state
				if(led_data[i].mPin & (1 << PULSE_DIRECTION))
				{
					led_data[i].mBrightness++;
				}
				else
				{
					if(--led_data[i].mBrightness == 0)
					{
						led_data[i].mTime = getTime();
					}
				}
				break;
		}
	}
}


void setup()
 {
	//set all LED pins as outputs
	DDRB |= ALL_LEDS;

	// COM0A1 - COM0A0 (Set OC0A on Compare Match, clear OC0A at TOP)
	// WGM01 - WGM00 (set fast PWM)
//	TCCR0A |= (1 << COM0A1) | (1 << COM0A0) | (1 << WGM01) | (1 << WGM00);

	// initialize Output Compare Register A to 0
//	OCR0A = 250;

	// Start timer at Fcpu / 8
//	TCCR0B |= (1 << CS01);
}

int main(void)
{
	setup();

	//infinite loop
	while(1)
	{
		updateLedState();
		pulseLeds();
	}
}
