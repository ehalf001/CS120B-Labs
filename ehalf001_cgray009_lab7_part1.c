/* Partner 1 Name & E-mail: Emanuel Halfon ehalf001@ucr.edu
* Partner 2 Name & E-mail:  Christian Grayson cgray009@ucr.edu
* Lab Section: 024
* Assignment: Lab # 7 Exercise # 1
* Exercise Description: Adding the first bit and the second bit of an input.
*
* I acknowledge all content contained herein, excluding template or example
* code, is my own original work.
*/

#include <avr/io.h>
#include "io.c"

#define A0 (~PINA & 0x01)
#define A1 ((~PINA >> 1) & 0x01)

enum  States {Start, Inter, Incre, Incre_ON, Decre, Decre_ON} state;

int main(void)
{
	PORTA = 0xFF;
	DDRA = 0x00;
	DDRB = 0xFF; PORTB = 0x00;
	DDRC = 0xFF; PORTC = 0x00; // LCD data lines
	DDRD = 0xFF; PORTD = 0x00; // LCD control lines
	
	// Initializes the LCD display
	LCD_init();
	
	// Starting at position 1 on the LCD screen, writes Hello World

	unsigned char k = 0;

	//LCD_DisplayString(1, "Hello World");

	while(1) 
	{
		//LCD_ClearScreen();
		LCD_WriteData( k + '0' );
		switch(state) //Transitions
		{
			case Start:
				state = Inter;
				break;
			case Inter:
				if(A0 == A1)
				{
					state = Inter;
				}
				else if(A0)
				{
					state = Incre_ON;
				}
				else
				{
					state = Decre_ON;
				}
				break;
			case Incre:
				state = Inter;
				break;
			case Incre_ON:
				if(!A0)
				{
					state = Incre;
				}
				else
				{
					state = Incre_ON;
				}
				break;
			case Decre:
				state = Inter;
				break;
			case Decre_ON:
				if(!A1)
				{
					state = Decre;
				}
				else
				{
					state = Decre_ON;
				}
				break;
			default:
				state = Inter;
		}

		switch(state) //Actions
		{
			case Start:
				PORTB = 0x01;
				break;
			case Inter:
				PORTB = 0x02;
				break;
			case Incre_ON:
				PORTB = 0x03;
				break;
			case Incre:
				PORTB = 0x04;
				if(k < 9)
				{
					k = k + 0x01;
				}
				break;
			case Decre_ON:
				PORTB = 0x05;
				break;
			case Decre:
				PORTB = 0x06;
				if(k > 0)
				{
					k = k - 0x01;
				}
				break;
			default:
				PORTB = 0x07;
				k = k;
		}
	
	}
}
