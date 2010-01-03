/*
 * FireflyJar.c
 *
 *  Created on: Jan 1, 2010
 *      Author: Paul
 */
#import <util/delay.h>
#import <avr/io.h>
#import <avr/interrupt.h>

#ifndef F_CPU
	#define F_CPU 9600000
#endif

//LED pin defs
#define LED1			PB0
#define LED2			PB1
#define LED3			PB2
#define LED4			PB3
#define LED5			PB4
#define N_LEDS			5
#define ALL_LEDS (1 << LED1) | (1 << LED2) | (1 << LED3) | (1 << LED4) | (1 << LED5)

//Max value for brightness
#define MAX				100

#define PULSE_UP 1
#define PULSE_DOWN 0

volatile unsigned char buffer[N_LEDS];

//counter for use in updateLedState() and pulseLeds()
unsigned char i,j;

//structure for storing data on each LED
struct ledData {
	unsigned char mBrightness;
	unsigned int mTime;
	unsigned char mPin;
	unsigned char mPulseDirection;
};

//set up some initial values
struct ledData led_data[] = {
		{0, 250,  LED1, 0},
		{MAX, 10, LED2, 0},
		{0, 200, LED3, 0},
		{10, 250, LED4, 1},
		{200, 100,  LED5, 1}
};

//TODO return a random time interval from 1 to 255
int getTime()
{
	return 50;
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
					led_data[i].mPulseDirection = PULSE_DOWN;
				}
				break;
			case 0://led is off
				if(--led_data[i].mTime == 0)
				{
					//increment the brightness,this puts the LED in
					//a pulse state
					led_data[i].mBrightness++;

					//specify the "up" direction for pulsing
					led_data[i].mPulseDirection = PULSE_UP;

					//set the ON time
					led_data[i].mTime = getTime();
				}
				break;
			default: //pulse state
				if(led_data[i].mPulseDirection == PULSE_UP)
				{
					led_data[i].mBrightness++;
				}
				else
				{
					if(--led_data[i].mBrightness == 0)
					{
						//set the OFF time
						led_data[i].mTime = getTime();
					}
				}
				break;
		}
	}
}

void init_timers()
{
	//TIFR0  = (1 << TOV0);          // clear interrupt flag
	TIMSK0 = (1 << TOIE0);         // enable overflow interrupt
	TCCR0B = (1 << CS00);          // start timer, no prescale

	sei();                      // enable interrupts
}

void init_clock()
{
	//The ATtiny13 chip comes with the CKDIV8 fuse bit programmed
	//by default.  I want to run at 9.6MHz, which means we have to
	//correct the Clock pre-scaling - see pg 28 of the datasheet
	//1. enable change of CLKPS bits by writing a logical '1' to CLKPCE
	//(Clock Prescaler Change enable)
	CLKPR |= (1 << CLKPCE);

	//2. update the prescaler bits (this has to happen within 4 cycles)
	CLKPR = 0;//clear all bits (this does not affect the CLKPCE bit - see the datasheet).

	//now we should be running at 9.6MHz
}

void init_io()
{
	//set all LED pins as outputs
	DDRB |= ALL_LEDS;
	PORTB &= ~ALL_LEDS; //off to start}
}

void setup()
 {
	init_clock();
	init_io();
	init_timers();
}

int main(void)
{
	setup();

	//infinite loop
	while(1)
	{
		updateLedState();
		_delay_ms(10);
	}
}

/*
 * Timer/Counter overflow interrupt. This is called each time
 * the counter overflows (255 counts/cycles).
 */
ISR(TIM0_OVF_vect)
{
	//static variables maintain state from one call to the next
	static unsigned char sPortBmask = ALL_LEDS;
	static unsigned char sCounter = 0xFF;

	//set port pins straight away (no waiting for processing)
	PORTB = sPortBmask;

	//this counter will overflow back to 0 after reaching 255.
	//So we end up adjusting the LED states for every 256 overflows.
	if(++sCounter == 0)
	{
		for(i = 0; i < N_LEDS; i++)
		{
			buffer[i] = led_data[i].mBrightness;
		}
		//set all pins to high (they are cleared as needed in the next step)
		sPortBmask = ALL_LEDS;
	}
	//this loop is considered for every overflow interrupt.
	//this is the software PWM.
	if(buffer[0] == sCounter) sPortBmask &= ~(1 << led_data[0].mPin);
	if(buffer[1] == sCounter) sPortBmask &= ~(1 << led_data[1].mPin);
	if(buffer[2] == sCounter) sPortBmask &= ~(1 << led_data[2].mPin);
	if(buffer[3] == sCounter) sPortBmask &= ~(1 << led_data[3].mPin);
	if(buffer[4] == sCounter) sPortBmask &= ~(1 << led_data[4].mPin);
}
