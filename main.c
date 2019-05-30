/* Partner 1 Name & E-mail: Emanuel Halfon ehalf001@ucr.edu
* Partner 2 Name & E-mail:  Christian Grayson cgray009@ucr.edu
* Lab Section: 024
* Assignment: Lab # 10 Exercise # 1
* Exercise Description: Adding the first bit and the second bit of an input.
*
* I acknowledge all content contained herein, excluding template or example
* code, is my own original work.
*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h> 
#include "ucr/keypad.h"
#include "ucr/io.c"
#include <stdio.h>

#define A0 (~PINA & 0x01)
#define A1 ((~PINA >> 1) & 0x01)
#define A2 ((~PINA >> 2) & 0x01)

volatile unsigned char TimerFlag = 0; // TimerISR() sets this to 1. C programmer should clear to 0.
volatile unsigned char port_B = 0x00;
// Internal variables for mapping AVR's ISR to our cleaner TimerISR model.
unsigned long _avr_timer_M = 1; // Start count from here, down to 0. Default 1 ms.
unsigned long _avr_timer_cntcurr = 0; // Current internal count of 1ms ticks
volatile unsigned short randomValues;

const unsigned long PERIOD = 10;

int randomNumGen(int state)
{
	static unsigned char randVals = 0x01;
	randVals += randVals;
	randomValues = randVals;
	return state;	
}



unsigned char DisplayVal(void)
{
	unsigned char port_B;
	unsigned char x;
	x = GetKeypadKey();
	switch (x)
	{
		case '\0': port_B = 'L'; break; // All 5 LEDs on
		case '0':  port_B = '0'; break;
		case '1':  port_B = '1'; break; // hex equivalent
		case '2':  port_B = '2'; break;
		case '3':  port_B = '3'; break;
		case '4':  port_B = '4'; break;
		case '5':  port_B = '5'; break;
		case '6':  port_B = '6'; break;
		case '7':  port_B = '7'; break;
		case '8':  port_B = '8'; break;
		case '9':  port_B = '9'; break;
		case 'A':  port_B = 'A'; break;
		case 'B':  port_B = 'B'; break;
		case 'C':  port_B = 'C'; break;
		case 'D':  port_B = 'D'; break;
		case '*':  port_B = '*'; break;
		case '#':  port_B = '#'; break;
		default:   port_B = 'H'; break; // Should never occur. Middle LED off.
	}
	return port_B;
}


void TimerOn() {
	// AVR timer/counter controller register TCCR1
	TCCR1B = 0x0B;
	// AVR output compare register OCR1A.
	OCR1A = 125;	// Timer interrupt will be generated when TCNT1==OCR1A
	TIMSK1 = 0x02; // bit1: OCIE1A -- enables compare match interrupt

	//Initialize avr counter
	TCNT1=0;

	_avr_timer_cntcurr = _avr_timer_M;
	SREG |= 0x80; // 0x80: 1000000
}

void TimerOff() {
	TCCR1B = 0x00; // bit3bit1bit0=000: timer off
}


typedef struct Task {
	int state; // Task’s current state
	unsigned long period; // Task period
	unsigned long elapsedTime; // Time elapsed since last task tick
	int (*TickFct)(int); // Task tick function
} task;

const unsigned char tasksSize = 2;
task tasks[2];

enum MenuStates {Start, Menu, Game_Start};

int TickMenu(int state)
{
	unsigned char x;
	unsigned char* string = "1.Start";
	unsigned char c = 17;
	static unsigned char j = 0x00;
	unsigned char randVals[3];
	switch(state)
	{
		case Start:
			LCD_DisplayString(6, " Menu");
			while(*string)
			{
				LCD_Cursor(c++);
				LCD_WriteData(*string++);
			}
			state = Menu;
			break;
		case Menu:
			x = GetKeypadKey();
			if(x == '1')
			{
				state = Game_Start;
				break;
			}
			else
			{
				state = Menu;
			}
			break;
		case Game_Start:
			state = Game_Start;
			break;
		default:
			state = Menu;
	}
	switch(state)
	{
		case Game_Start:
		if(j < 3)
		{
			randVals[j] = randomValues % 5;
			PORTB = randVals;
			j++;
		}
		else
		{
			j = 0x00;
		}	
		LCD_DisplayString(1, "Loading");
		break;
	}
	return state;
}


void TimerISR()
{
	unsigned char i;
	for (i = 0;i < tasksSize;++i)
	{
		if (tasks[i].elapsedTime >= tasks[i].period)
		{
			tasks[i].state = tasks[i].TickFct(tasks[i].state);
			tasks[i].elapsedTime = 0;
		}
		tasks[i].elapsedTime += PERIOD;
	}
}

ISR(TIMER1_COMPA_vect) {
	// CPU automatically calls when TCNT1 == OCR1 (every 1 ms per TimerOn settings)
	_avr_timer_cntcurr--; // Count down to 0 rather than up to TOP
	if (_avr_timer_cntcurr == 0) { // results in a more efficient compare
		TimerISR(); // Call the ISR that the user uses
		_avr_timer_cntcurr = _avr_timer_M;
	}
}

// Set TimerISR() to tick every M ms
void TimerSet(unsigned long M) {
	_avr_timer_M = M;
	_avr_timer_cntcurr = _avr_timer_M;
}


int main(void)
{
	DDRB = 0xFF; PORTB = 0x00;
	DDRA = 0xF0; PORTA = 0x0F;
	DDRC = 0xFF; PORTC = 0x00;
	DDRD = 0xFF; PORTD = 0x00;
	unsigned char i = 0;
	tasks[i].state = Start;
	tasks[i].period = 1000;
	tasks[i].elapsedTime = 0;
	tasks[i].TickFct = &TickMenu;
	i++;
	tasks[i].state = Menu;
	tasks[i].period = 10;
	tasks[i].elapsedTime = 0;
	tasks[i].TickFct = &randomNumGen;
	TimerSet(PERIOD);
	TimerOn();
	LCD_init();
	LCD_ClearScreen();
	while(1) 
	{
		continue;
	}
	return 0;
}
