/* Partner 1 Name & E-mail: Emanuel Halfon ehalf001@ucr.edu
* Partner 2 Name & E-mail:  Christian Grayson cgray009@ucr.edu
* Lab Section: 024
* Assignment: Lab # 9 Exercise # 2
* Exercise Description: Adding the first bit and the second bit of an input.
*
* I acknowledge all content contained herein, excluding template or example
* code, is my own original work.
*/

#include <avr/io.h>
#include <avr/interrupt.h>

#define A0 (~PINA & 0x01)
#define A1 ((~PINA >> 1) & 0x01)
#define A2 ((~PINA >> 2) & 0x01)

void set_PWM(double frequency) {
	static double current_frequency; // Keeps track of the currently set frequency
	// Will only update the registers when the frequency changes, otherwise allows
	// music to play uninterrupted.
	if (frequency != current_frequency) {
		if (!frequency) { TCCR3B &= 0x08; } //stops timer/counter
		else { TCCR3B |= 0x03; } // resumes/continues timer/counter
		
		// prevents OCR3A from overflowing, using prescaler 64
		// 0.954 is smallest frequency that will not result in overflow
		if (frequency < 0.954) { OCR3A = 0xFFFF; }
		
		// prevents OCR0A from underflowing, using prescaler 64					// 31250 is largest frequency that will not result in underflow
		else if (frequency > 31250) { OCR3A = 0x0000; }
		
		// set OCR3A based on desired frequency
		else { OCR3A = (short)(8000000 / (128 * frequency)) - 1; }

		TCNT3 = 0; // resets counter
		current_frequency = frequency; // Updates the current frequency
	}
}

void PWM_on() {
	TCCR3A = (1 << COM3A0);
	// COM3A0: Toggle PB3 on compare match between counter and OCR0A
	TCCR3B = (1 << WGM32) | (1 << CS31) | (1 << CS30);
	// WGM02: When counter (TCNT0) matches OCR0A, reset counter
	// CS01 & CS30: Set a prescaler of 64
	set_PWM(0);
}

void PWM_off() {
	TCCR3A = 0x00;
	TCCR3B = 0x00;
}


int main(void)
{
	double frequencies[] = {261.63, 293.66, 329.63, 349.23, 392.00, 440.00, 493.88, 523.25};
	DDRB = 0xFF; // Set port B to output
	PORTB = 0x00; // Init port B to 0s
	DDRD = 0xFF; // Set port B to output
	PORTD = 0x00; // Init port B to 0s
	PORTA = 0xFF;
	DDRA = 0x00;
	PWM_on();
	double curr_freq = frequencies[0];
	unsigned char ison = 0x00;
	int i = 0;
	set_PWM(curr_freq);
	while(1)
	{
		if(A0)
		{
			if(!ison)
			{
				ison = 0x01;
				set_PWM(curr_freq);
				while(A0);
			}
			else
			{
				set_PWM(0);
				ison = 0x00;
				while(A0);
			}
		}
		else if(A1)
		{
			if(i < 7)
			{
				i++;
				curr_freq = frequencies[i];
				set_PWM(curr_freq);
			}
			while(A1);
		}
		else if(A2)
		{
			if(i > 0)
			{
				i--;
				curr_freq = frequencies[i];
				set_PWM(curr_freq);
			}
			while(A2);
		}
	}
}