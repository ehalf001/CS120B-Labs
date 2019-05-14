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
#include "io.c"

#define A0 (~PINA & 0x01)
#define A1 ((~PINA >> 1) & 0x01)
#define A2 ((~PINA >> 2) & 0x01)

volatile unsigned char TimerFlag = 0; // TimerISR() sets this to 1. C programmer should clear to 0.
volatile unsigned char port_B = 0x00;
// Internal variables for mapping AVR's ISR to our cleaner TimerISR model.
unsigned long _avr_timer_M = 1; // Start count from here, down to 0. Default 1 ms.
unsigned long _avr_timer_cntcurr = 0; // Current internal count of 1ms ticks

const unsigned long PERIOD = 2;

unsigned char SetBit(unsigned char x, unsigned char k, unsigned char b) {
	return (b ? x | (0x01 << k) : x & ~(0x01 << k));
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

const unsigned char tasksSize = 3;
task tasks[3];
enum BL_States { BL_SMStart, BL_LEDOff, BL_LEDOn};
enum TL_States{ TL_SMStart, TL_Seq0, TL_Seq1, TL_Seq2 };
enum TS_States { TS_SMStart, TS_Off, TS_On};
int TickFct_BlinkLED(int state)
{
	switch(state)
	{
		case BL_SMStart:
		state = BL_LEDOff;
		break;
		case BL_LEDOff:
		state = BL_LEDOn;
		break;
		case BL_LEDOn:
		state = BL_LEDOff;
		break;
		default:
		state = BL_LEDOff;
		break;

	}
	switch(state)
	{
		case BL_LEDOff:
		port_B = SetBit(port_B, 0, 0);
		break;
		case BL_LEDOn:
		port_B = SetBit(port_B, 0, 1);

		break;

	}
	return state;
}
int TickFct_ThreeLED(int state)
{
	switch(state)
	{
		case TL_SMStart:
		state = TL_Seq0;
		break;
		case TL_Seq0:
		state = TL_Seq1;
		break;
		case TL_Seq1:
		state = TL_Seq2;
		break;
		case TL_Seq2:
		state = TL_Seq0;
		break;
		default:
		state = TL_Seq0;
		break;

	}
	switch(state)
	{
		case TL_Seq0:
		port_B = SetBit(port_B, 1, 1);
		port_B = SetBit(port_B, 2, 0);
		port_B = SetBit(port_B, 3, 0);
		break;
		case TL_Seq1:
		port_B = SetBit(port_B, 1, 0);
		port_B = SetBit(port_B, 2, 1);
		port_B = SetBit(port_B, 3, 0);
		break;
		case TL_Seq2:
		port_B = SetBit(port_B, 1, 0);
		port_B = SetBit(port_B, 2, 0);
		port_B = SetBit(port_B, 3, 1);
		break;

	}
	return state;
}

int TickFct_ToggleSpeaker(int state)
{
	switch(state)
	{
		case TS_SMStart:
			state = TS_Off;
			break;
		case TS_Off:
			state = TS_On;
			break;
		case TS_On:
			state = TS_Off;
			break;
		default:
			state = TS_Off;
			break;

	}
	switch(state)
	{
		case TS_Off :
			port_B = SetBit(port_B, 4, 0);
			break;
		case TS_On:
			if(A2 == 1)
			{
				port_B = SetBit(port_B, 4, 1);
			}
			else
			{
				port_B = SetBit(port_B, 4, 0);
			}
			break;
		default:
			port_B = SetBit(port_B, 4, 0);
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
	DDRA = 0x00; PORTA = 0xFF;
	unsigned char i = 0;
	tasks[i].state = BL_SMStart;
	tasks[i].period = 1000;
	tasks[i].elapsedTime = 0;
	tasks[i].TickFct = &TickFct_BlinkLED;
	i++;
	tasks[i].state = TL_SMStart;
	tasks[i].period = 300;
	tasks[i].elapsedTime = 0;
	tasks[i].TickFct = &TickFct_ThreeLED;
	i++;
	tasks[i].state = TS_SMStart;
	tasks[i].period = 2;
	tasks[i].elapsedTime = 0;
	tasks[i].TickFct = &TickFct_ToggleSpeaker;
	TimerSet(PERIOD);
	TimerOn();
	while(1) { PORTB = port_B; }
	return 0;
}
