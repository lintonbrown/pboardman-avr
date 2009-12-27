#include <avr/io.h>
#include <util/delay.h>
#define ADC_ENA_BIT 2 // use PB4 to enable ldr, pin 3
#define LED_BIT 1 // pin 6

uint16_t readAdc(uint8_t channel)
{
	//select ADC must be 0-7
	channel &= 7;
	ADMUX |= channel;
	//start a conversion
	ADCSRA |= (1<<ADSC);

	while(ADCSRA & (1<<ADSC));

	//clear ADIF by writing 1 to it
	//yes, this results in setting the bit to '0'.
	ADCSRA |= (1<<ADIF);

	return(ADCH);
}

int main(void)
{
	uint8_t i = 0;
	DDRB |= (1 << LED_BIT)
	| (1 << ADC_ENA_BIT); //enable current for LDR voltage divider

	PORTB |= (1 << ADC_ENA_BIT); // power on teh voltage divider


	//enable ADC and division factor of 8
	ADCSRA |= (1 << ADEN)| //enable ADC
	(1 << ADPS1)| (1 << ADPS2) | (1 << ADPS0);// presaler 128 -> 75K samples at 9.6 MHz

	// select ADC channel
	ADMUX |= (0 << REFS0)|// set aref=Vcc
	(1 << ADLAR)| //ADC output as 8-bit, left adjusted
	(1 << MUX1); //select channel 2

	//flash five times to signal on.
	for(i=0;i<5;i++){
		PORTB |= (1 << LED_BIT); //ledbit set to on
		_delay_ms(250);
		PORTB &= ~(1 << LED_BIT); //ledbit set to off
		_delay_ms(250);
	}

	uint16_t adcResult;

	//grab the first reading and discard
	adcResult = readAdc(ADC_ENA_BIT);
	while (1)
	{
		adcResult = readAdc(ADC_ENA_BIT);

		//turn on the led
		PORTB |= (1 << LED_BIT);

		for(i=0; i < adcResult; i++){
			_delay_ms(10);
		}

		//turn off the led
		PORTB &= ~(1 << LED_BIT);
		for(i=0; i < adcResult; i++){
			_delay_ms(10);
		}
	}
}
