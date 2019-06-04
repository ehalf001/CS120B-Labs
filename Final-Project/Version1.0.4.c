#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include "ucr/keypad.h"
#include "ucr/speaker.h"
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
unsigned short randomValues;

unsigned char IsActives[3];

const unsigned long PERIOD = 50;

unsigned char Games[3];
unsigned char GamePOINTER;


unsigned char GameOver;

int randomNumGen(int state)
{
	unsigned char randVals = randomValues + 0x01;
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
	int isActive;
} task;



const unsigned char tasksSize = 3;
task tasks[3];


enum MenuStates {Start, Menu, Loading};

int TickMenu(int state)
{
	unsigned char x;
	unsigned char* string = "1.Start";
	unsigned char c = 17;
	unsigned char randInt = 0;
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
				state = Loading;
				break;
			}
			else
			{
				state = Menu;
			}
			break;
		case Loading:
			state = Loading;
			break;
		default:
			state = Start;
	}
	switch(state)
	{
		case Loading:
			if(Games[0] == 10)
			{
				randInt = randomValues;
				Games[0] = randInt*7 % 5;
				Games[1] = randInt*13 % 5;
				Games[2] = (randInt + Games[1] + Games[2]) % 5;			
			}
			else
			{
				IsActives[0] = 0;
				IsActives[2] = 1;
			}
			LCD_DisplayString(1, "Loading");
			break;
	}
	return state;
}

enum MemStates {Start_light, Instructions, levelO, levelO_display, levelO_play, Mem_Win};


int MemLightsTck(int state)
{
	unsigned char randInt = 0x00;
	static unsigned char lev_One[3]; 
	static unsigned char i;
	unsigned char x;
	unsigned char y;
	switch(state)
	{
		case Start_light:
			LCD_ClearScreen();
			LCD_DisplayString(1, "Memory          Light Game");
			state = Instructions;
			PORTB = SetBit(PORTB, 3, 1);
			break;
		case Instructions:
			if(GetBit(PORTB, 0))
			{
				set_PWM(0);
				PORTB = SetBit(PORTB, 0, 0);
				LCD_ClearScreen();
				LCD_DisplayString(1, "Level ");
				LCD_WriteData('1' + GamePOINTER);
				state = levelO;
			}
			else
			{
				state = Instructions;
			}
			break;
		case levelO:
			state = levelO_display;
			break;
		case levelO_display:
			if(i < 3)
			{
				state = levelO_display;
			}
			else
			{
				PORTB = SetBit(PORTB, 5, 0) & SetBit(PORTB, 7, 0);
				PORTD = 0x00;
				i = 0;
				state = levelO_play;
			}
			break;
		case levelO_play:
			if(i < 3)
			{
				state = levelO_play;
			}
			else
			{
				PORTB = SetBit(PORTB, 5, 0) & SetBit(PORTB, 7, 0);
				PORTD = 0x00;
				i = 0;
				state = Mem_Win;
				LCD_ClearScreen();
				LCD_DisplayString(1, "Congratulations");
			}
			break;
		case Mem_Win:
			state = Mem_Win;
			break;
		default:
			state = Start_light;
	}
	switch(state)
	{
		case Instructions:
			if(GetBit(PORTB, 3))
			{
				set_PWM(frequencies[D]);
				PORTB = SetBit(PORTB, 3, 0);
				PORTB = SetBit(PORTB, 2, 1);
			}
			else if(GetBit(PORTB, 2))
			{
				set_PWM(frequencies[A]);
				PORTB = SetBit(PORTB, 2, 0);
				PORTB = SetBit(PORTB, 1, 1);
			}
			else if(GetBit(PORTB, 1))
			{
				set_PWM(frequencies[E]);
				PORTB = SetBit(PORTB, 1, 0);
				PORTB = SetBit(PORTB, 0, 1);
			}			
			break;
		case levelO:
			randInt = randomValues;
			lev_One[0] = randInt*7 % 8;
			lev_One[1] = randInt*13 % 8;
			lev_One[2] = (randInt + lev_One[1] + lev_One[0] + 1)% 8;
			i = 0;
			break;
		case levelO_display:
			PORTB = SetBit(PORTB, 5, 0) & SetBit(PORTB, 7, 0);
			PORTD = 0x00;
			switch(lev_One[i])
			{
				case 0: PORTB = SetBit(PORTB, 5, 1); break;
				case 1: PORTB = SetBit(PORTB, 7, 1); break;
				case 2: PORTD = SetBit(PORTD, 0, 1); break;
				case 3: PORTD = SetBit(PORTD, 1, 1); break;
				case 4: PORTD = SetBit(PORTD, 2, 1); break;
				case 5: PORTD = SetBit(PORTD, 3, 1); break;
				case 6: PORTD = SetBit(PORTD, 4, 1); break;
				case 7: PORTD = SetBit(PORTD, 5, 1); break;
			}
			i = i + 1;
			break;
		case levelO_play:
			PORTB = SetBit(PORTB, 5, 0) & SetBit(PORTB, 7, 0);
			PORTD = 0x00;
			y = x;
			while(y == x)
			{
				x = GetKeypadKey();
			}
			switch(x)
			{
				case '0': y = 0; PORTB = SetBit(PORTB, 5, 1); break;
				case '1': y = 1; PORTB = SetBit(PORTB, 7, 1); break;
				case '2': y = 2; PORTD = SetBit(PORTD, 0, 1); break;
				case '3': y = 3; PORTD = SetBit(PORTD, 1, 1); break;
				case '4': y = 4; PORTD = SetBit(PORTD, 2, 1); break;
				case '5': y = 5; PORTD = SetBit(PORTD, 3, 1); break;
				case '6': y = 6; PORTD = SetBit(PORTD, 4, 1); break;
				case '7': y = 7; PORTD = SetBit(PORTD, 5, 1); break;
				default: break;
			}
			if(lev_One[i] == y)
			{
				i++;
			}
			else if(lev_One[i] != y)
			{
				PORTB = SetBit(PORTB, 5, 0) & SetBit(PORTB, 7, 0);
				PORTD = 0x00;
				GameOver = 1;
				return state;
			}
			break;	
		case Mem_Win:
			GamePOINTER++;
			break;
		default:
			PORTB = PORTB;
	}
	return state;
}

void GameOvah(void)
{
	unsigned char x;
	GamePOINTER = 0;
	GameOver = 0;
	Games[0] = 10;
	Games[1] = 10;
	Games[2] = 10;
	IsActives[0] = 1;
	IsActives[1] = 1;
	IsActives[2] = 0;
	tasks[0].state = Start;
	tasks[2].state = Start_light;
	LCD_ClearScreen();
	LCD_DisplayString(1, "GAME OVAH       1.Try Again??");
	x = GetKeypadKey();
	while(x != '1')
	{
		x = GetKeypadKey();
	}
	
}

void WINSCREEN(void)
{
	unsigned char x;
	GamePOINTER = 0;
	GameOver = 0;
	Games[0] = 10;
	Games[1] = 10;
	Games[2] = 10;
	IsActives[0] = 1;
	IsActives[1] = 1;
	IsActives[2] = 0;
	tasks[0].state = Start;
	tasks[2].state = Start_light;
	LCD_ClearScreen();
	LCD_DisplayString(6, "YOU WIN    1.Play Again");
	x = GetKeypadKey();
	while(x != '1')
	{
		x = GetKeypadKey();
	}
	
}

void TimerISR()
{
	unsigned char i;
	if(GameOver == 1)
	{
		GameOvah();
	}
	if(GamePOINTER > 1)
	{
		WINSCREEN();
	}
	for (i = 0;i < tasksSize;++i)
	{
		if(IsActives[i] == 1 && tasks[i].isActive == 0)
		{
			tasks[i].isActive = 1;
		}
		else if(IsActives[i] == 0 && tasks[i].isActive == 1)
		{
			tasks[i].isActive = 0;
		}
		if ((tasks[i].elapsedTime >= tasks[i].period) && tasks[i].isActive == 1)
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
	PWM_on();
	GamePOINTER = 0;
	GameOver = 0;
	randomValues = 0x00;
	Games[0] = 10;
	Games[1] = 10;
	Games[2] = 10;
	IsActives[0] = 1;
	IsActives[1] = 1;
	IsActives[2] = 0;
	unsigned char i = 0;
	tasks[i].state = Start;
	tasks[i].period = 500;
	tasks[i].elapsedTime = 0;
	tasks[i].TickFct = &TickMenu;
	tasks[i].isActive = 1;
	i++;
	tasks[i].state = Menu;
	tasks[i].period = 101;
	tasks[i].elapsedTime = 0;
	tasks[i].TickFct = &randomNumGen;
	tasks[i].isActive = 1;
	i++;
	tasks[i].state = Start_light;
	tasks[i].period = 1000;
	tasks[i].elapsedTime = 0;
	tasks[i].TickFct = &MemLightsTck;
	tasks[i].isActive = 0;
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
