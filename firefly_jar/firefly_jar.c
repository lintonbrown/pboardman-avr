/*
 * firefly_jar.c
 *
 *  Created on: Mar 19, 2009
 *      Author: Paul
 */

#include <stdlib.h>
#include <avr/io.h>
#include <inttypes.h>

//Hardware definitions
#define LED1      PB0
#define LED2      PB1
#define LED3      PB2
#define LED4      PB3
#define LED5      PB4
#define LED6      PB5

int leds[6] = {LED1, LED2, LED3, LED4, LED5, LED6};
int values[6] = {0,0,0,0,0,0};
int states[6] = {0,0,0,0,0,0};
unsigned char led = 0;

//max level
#define V_MAX 255

//pulse state
#define OFF 0
#define PULSE_ON 1
#define ON 2
#define PULSE_OFF 255

void ledCheck(int led, unsigned char value)
{
	if (value > values[led]){
		PORTB &=~ _BV(led);
	}else{
		PORTB |= _BV(led);
	}
}

void updateState()
{
	for(led = 1; led < 5; led++)
	{
		switch(states[led])
		{
			case OFF:
				//RAND_MAX is defined as 0x7FFF
				if(random() > 0x6FFF)
					states[led] = PULSE_ON;
				break;
			case PULSE_ON:
				if(++values[led] == V_MAX)
				{
					states[led] = ON;
				}
				break;
			case ON:
				states[led]++;
				break;
			case PULSE_OFF:
				if(--values[led] == 0)
				{
					states[led] = OFF;
				}
				break;
		}
	}
}

int main(void){
	unsigned char i=0;
	unsigned char j=0;
	//Set pins to output
	DDRB |= _BV(DDB0);
	DDRB |= _BV(DDB1);
	DDRB |= _BV(DDB2);
	DDRB |= _BV(DDB3);
	DDRB |= _BV(DDB4);

	while (1) {
		//Software PWM
		for (j=1; j<5;j++)
		{
			ledCheck(j, i);
		}

		if (i==0){ //After blinking LEDs 255 times
			updateState();
		}
		i++;
	}
	return 0; //Will never get here
}
