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

unsigned char IsActives[6];

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



const unsigned char tasksSize = 6;
task tasks[6];


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
			lev_One[1] = randInt*11 % 8;
			lev_One[2] = (randInt + lev_One[1] + lev_One[0] + 1) % 8;
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
				case '1': y = 0; PORTB = SetBit(PORTB, 5, 1); break;
				case '2': y = 1; PORTB = SetBit(PORTB, 7, 1); break;
				case '3': y = 2; PORTD = SetBit(PORTD, 0, 1); break;
				case '4': y = 3; PORTD = SetBit(PORTD, 1, 1); break;
				case '5': y = 4; PORTD = SetBit(PORTD, 2, 1); break;
				case '6': y = 5; PORTD = SetBit(PORTD, 3, 1); break;
				case '7': y = 6; PORTD = SetBit(PORTD, 4, 1); break;
				case '8': y = 7; PORTD = SetBit(PORTD, 5, 1); break;
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
			IsActives[2] = 0;
			IsActives[3] = 1;
			GamePOINTER++;
			break;
		default:
			PORTB = PORTB;
	}
	return state;
}


enum NoteStates {Start_Note, Generate, levelT, levelT_display, levelT_play, Notes_Win};

int NotesTck(int state)
{
	unsigned char randInt = 0x00;
	static unsigned char lev_two[3];
	static unsigned char i;
	unsigned char x;
	unsigned char y;
	unsigned char c = 17;
	unsigned char* string = "0.Play 1-4.A-D";
	switch(state)
	{
		case Start_Note:
			LCD_ClearScreen();
			LCD_DisplayString(1, "Note Guess Game");
			state = Generate;
			PORTB = SetBit(PORTB, 3, 1);
			break;
		case Generate:
			if(GetBit(PORTB, 0))
			{
				set_PWM(0);
				PORTB = SetBit(PORTB, 0, 0);
				LCD_ClearScreen();
				LCD_DisplayString(1, "Level ");
				LCD_WriteData('1' + GamePOINTER);
				while(*string)
				{
					LCD_Cursor(c++);
					LCD_WriteData(*string++);
				}
				state = levelT;
			}
			else
			{
				state = Generate;
			}
			break;
		case levelT:
			state = levelT_display;
			break;
		case levelT_display:
			if( i < 3)
			{
				state = levelT_play;
			}
			else
			{
				LCD_DisplayString(1, "Congratulations!");
				state = Notes_Win;
			}
			break;
		case levelT_play:
			set_PWM(0);
			PORTB = SetBit(PORTB, 0, 0) & SetBit(PORTB, 1, 0) & SetBit(PORTB, 2, 0) & SetBit(PORTB, 3, 0) & SetBit(PORTB, 4, 0);
			x = 0;
			y = x;
			while(y == x)
			{
				x = GetKeypadKey();
			}
			switch(x)
			{
				case '0': y = F;  break;
				case '1': y = G;  break;
				case '2': y = C5; break;
				case '3': y = 15;  break;
				case '4': y = 10; break;
				case 'A': y = A;  break;
				case 'B': y = B;  break;
				case 'C': y = C4; break;
				case 'D': y = D;  break;
				default: break;
			}
			if(y == F)
			{
				PORTB = SetBit(PORTB, 4, 1);
				state = levelT_display;
			}
			else if(y == G)
			{
				PORTB = SetBit(PORTB, 0, 1);
				set_PWM(frequencies[A]);
			}
			else if(y == C5)
			{
				PORTB = SetBit(PORTB, 1, 1);
				set_PWM(frequencies[B]);
			}
			else if(y == 15)
			{
				PORTB = SetBit(PORTB, 2, 1);
				set_PWM(frequencies[C4]);
			}
			else if(y == 10)
			{
				PORTB = SetBit(PORTB, 3, 1);
				set_PWM(frequencies[D]);
			}
			else if(lev_two[i] == y)
			{
				if(i == 2)
				{
					state = Notes_Win;
				}
				i++;
				state = levelT_display;
			}
			else if(lev_two[i] != y)
			{
				PORTB = SetBit(PORTB, 0, 0) & SetBit(PORTB, 1, 0) & SetBit(PORTB, 2, 0) & SetBit(PORTB, 3, 0) & SetBit(PORTB, 4, 0);
				GameOver = 1;
				return state;
			}
			else
			{
				state = levelT_play;
			}
			break;
		case Notes_Win:
			state =  Notes_Win;
			break;
		default:
			state = Start_Note;
	}
	switch(state)
	{
		case Generate:
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
		case levelT:
			randInt = randomValues;
			lev_two[0] = randInt*7 % 4;
			lev_two[1] = randInt*13 % 4;
			lev_two[2] = (randInt + lev_two[1] + lev_two[0] + 1)% 4;
			for(i = 0; i < 3; i++)
			{
				switch(lev_two[i])
				{
					case 0: lev_two[i] = A;  break;
					case 1: lev_two[i] = B;  break;
					case 2: lev_two[i] = C4; break;
					case 3: lev_two[i] = D;  break;
				}
			}
			i = 0;
			break;
		case levelT_display:
			if(i < 3)
			{
				set_PWM(frequencies[lev_two[i]]);
			}
			else
			{
				i = 0;
				state = Notes_Win;
			}
			break;
		case levelT_play:
			break;
		case Notes_Win:
			PORTB = SetBit(PORTB, 0, 0) & SetBit(PORTB, 1, 0) & SetBit(PORTB, 2, 0) & SetBit(PORTB, 3, 0) & SetBit(PORTB, 4, 0);
			IsActives[3] = 0;
			IsActives[4] = 1;
			GamePOINTER++;
			break;
		default:
			PORTB = PORTB;
	}
	return state;
}

enum FranStates {Start_fran, Gener, levelTh, levelTh_display, levelTh_play, Fran_Win};

int FranchiseTck(int state)
{
	unsigned char randInt = 0x00;
	static unsigned char lev_three[3];
	static unsigned char i;
	unsigned char x;
	unsigned char y;
	unsigned char c = 17;
	unsigned char j;
	unsigned char* string;
	unsigned char* franchise = "1.Zelda 2.Pacman3.Mario 4.Sonic";
	unsigned char* fran2 = "5.Mega Man";
	unsigned char* keywords[] = {"Hero", "Sword", "Time", "Ghost", "Cherries", "Maze", "Pipe", "Castle", "Mushroom", "Hedgehog", "Speed", "Rings", "Robot", "Cannon", "X"};
	switch(state)
	{
		case Start_fran:
			LCD_ClearScreen();
			LCD_DisplayString(1, "Color Franchise Game");
			state = Start_fran;
			PORTB = SetBit(PORTB, 3, 1);
			break;
		case Gener:
			if(GetBit(PORTB, 0))
			{
				set_PWM(0);
				PORTB = SetBit(PORTB, 0, 0);
				state = levelTh;
			}
			else
			{
				state = Generate;
			}
			break;
		case levelTh:
			LCD_ClearScreen();
			LCD_DisplayString(1, "Level ");
			LCD_WriteData('1' + GamePOINTER);
			state = levelTh_display;
			break;
		case levelTh_display:
			if( i < 3)
			{
				state = levelTh_play;
			}
			else
			{
				LCD_DisplayString(1, "Congratulations!");
				state = Fran_Win;
			}
			break;
		case levelTh_play:
			set_PWM(0);
			PORTB = SetBit(PORTB, 0, 0) & SetBit(PORTB, 1, 0) & SetBit(PORTB, 2, 0) & SetBit(PORTB, 3, 0) & SetBit(PORTB, 4, 0);
			x = 0;
			y = x;
			while(y == x)
			{
				x = GetKeypadKey();
			}
			switch(x)
			{
				case '1': y = 1; break;
				case '2': y = 2; break;
				case '3': y = 3; break;
				case '4': y = 4; break;
				case '5': y = 5; break;
				default: break;
			}
			switch(y)
			{
				case 1: PORTB = SetBit(PORTB, 0, 1);  break;
				case 2: PORTB = SetBit(PORTB, 1, 1);  break;
				case 3: PORTB = SetBit(PORTB, 2, 1);  break;
				case 4: PORTB = SetBit(PORTB, 3, 1);  break;
				case 5: PORTB = SetBit(PORTB, 4, 1);  break;
				default: break;	
			}
		    if(y == 1 && lev_three[i] < 3)
			{
				i++;
				state = levelTh_display;
			}
			else if(y == 2 && (lev_three[i] < 6 && lev_three[i] > 2))
			{
				i++;
				state = levelTh_display;
			}
			else if(y == 3 && (lev_three[i] < 9 && lev_three[i] > 5))
			{
				i++;
				state = levelTh_display;
			}
			else if(y == 4 && (lev_three[i] < 12 && lev_three[i] > 8))
			{
				i++;
				state = levelTh_display;
			}
			else if(y == 5 && (lev_three[i] > 11))
			{
				i++;
				state = levelTh_display;
			}									
			else if(lev_three[i] != y)
			{
				PORTB = SetBit(PORTB, 0, 0) & SetBit(PORTB, 1, 0) & SetBit(PORTB, 2, 0) & SetBit(PORTB, 3, 0) & SetBit(PORTB, 4, 0);
				GameOver = 1;
				return state;
			}
			else
			{
				state = levelTh_play;
			}
			break;
		case Fran_Win:
			state =  Fran_Win;
			break;
		default:
			state = Start_fran;
	}
	switch(state)
	{
		case Start_fran:
			state = Gener;
			break;
		case Gener:
			if(GetBit(PORTB, 3))
			{
				LCD_DisplayString(1, franchise);
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
				LCD_DisplayString(1, fran2);
				set_PWM(frequencies[E]);
				PORTB = SetBit(PORTB, 1, 0);
				PORTB = SetBit(PORTB, 0, 1);
			}
			break;
		case levelTh:
			randInt = randomValues;
			lev_three[0] = randInt*19 % 15;
			lev_three[1] = randInt*17 % 15;
			lev_three[2] = (randInt + lev_three[1] + lev_three[0] + 1) % 15;
			i = 0;
			break;
		case levelTh_display:
			if(i < 3)
			{
				c = 17;				
				for(j = 0; j < 33; j++)
				{
					LCD_Cursor(c + j);
					LCD_WriteData(' ');
				}
				string = keywords[lev_three[i]];
				while(*string)
				{
					LCD_Cursor(c++);
					LCD_WriteData(*string++);
				}
			}
			else
			{
				i = 0;
				state = Fran_Win;
			}
			break;
		case levelTh_play:
			break;
		case Fran_Win:
			PORTB = SetBit(PORTB, 0, 0) & SetBit(PORTB, 1, 0) & SetBit(PORTB, 2, 0) & SetBit(PORTB, 3, 0) & SetBit(PORTB, 4, 0);
			IsActives[4] = 0;
			IsActives[5] = 1;			
			GamePOINTER++;
			break;
		default:
			PORTB = PORTB;
	}
	return state;
}

enum HexStates {Start_hex, Generation, levelf, levelf_display, levelf_play, Bin_Win};

int Hex2BinTck(int state)
{
	unsigned char randInt = 0x00;
	static unsigned char lev_four[3];
	static unsigned char i;
	static unsigned char answer;
	unsigned char x;
	unsigned char y;
	unsigned char c = 17;
	static unsigned char j;
	unsigned char* string = "0x";
	switch(state)
	{
		case Start_hex:
			LCD_ClearScreen();
			LCD_DisplayString(1, "Hex to Binary   Game");
			state = Start_hex;
			PORTB = SetBit(PORTB, 3, 1);
			break;
		case Generation:
			if(GetBit(PORTB, 0))
			{
				set_PWM(0);
				PORTB = SetBit(PORTB, 0, 0);
				LCD_ClearScreen();
				LCD_DisplayString(1, "Level ");
				LCD_WriteData('1' + GamePOINTER);
				while(*string)
				{
					LCD_Cursor(c++);
					LCD_WriteData(*string++);
				}
				state = levelf;
			}
			else
			{
				state = Generation;
			}
			break;
		case levelf:
			state = levelf_display;
			break;
		case levelf_display:
			PORTB = SetBit(PORTB, 0, 0) & SetBit(PORTB, 1, 0) & SetBit(PORTB, 2, 0) & SetBit(PORTB, 3, 0) & SetBit(PORTB, 4, 0) & SetBit(PORTB, 5, 0) & SetBit(PORTB, 7, 0);
			PORTD = 0x00;
			if( i < 3)
			{
				state = levelf_play;
			}
			else
			{
				LCD_DisplayString(1, "Congratulations!");
				state = Bin_Win;
			}
			j = 0;
			break;
		case levelf_play:
			set_PWM(0);
			x = 0;
			y = x;
			while(y == x)
			{
				x = GetKeypadKey();
			}
			switch(x)
			{
				case '0': y = 0; break;
				case '1': y = 1;  PORTD = SetBit(PORTD, 3 - j, 1); break;
				default: break;
			}
			answer = SetBit(answer, 3 - j, y);
			j++;
			if(j > 3)
			{
				if(lev_four[i] == answer)
				{
					i++;
					state = levelf_display;
				}
				else if(lev_four[i] != answer)
				{
					PORTB = SetBit(PORTB, 0, 0) & SetBit(PORTB, 1, 0) & SetBit(PORTB, 2, 0) & SetBit(PORTB, 3, 0) & SetBit(PORTB, 4, 0) & SetBit(PORTB, 5, 0) & SetBit(PORTB, 7, 0);
					PORTD = 0x00;
					GameOver = 1;
					return state;
				}
			}
			else
			{
				state = levelf_play;
			}
			break;
		case Bin_Win:
			state =  Bin_Win;
			break;
		default:
			state = Start_hex;
	}
	switch(state)
	{
		case Start_hex:
			state = Generation;
			break;
			case Generation:
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
		case levelf:
			randInt = randomValues;
			lev_four[0] = randInt*19 % 15;
			lev_four[1] = randInt*17 % 15;
			lev_four[2] = (randInt + lev_four[1] + lev_four[0] + 1) % 15;
			i = 0;
			break;
		case levelf_display:
			if(i < 3)
			{
				c = 19;
				LCD_Cursor(c);
				switch(lev_four[i])
				{
					case 10: LCD_WriteData('A'); break;
					case 11: LCD_WriteData('B'); break;
					case 12: LCD_WriteData('C'); break;
					case 13: LCD_WriteData('D'); break;
					case 14: LCD_WriteData('E'); break;
					case 15: LCD_WriteData('F'); break;
					default: LCD_WriteData('0' + lev_four[i]); break;
				}
				
			}
			else
			{
				i = 0;
				state = Bin_Win;
			}
			answer = 0x00;
			break;
		case levelf_play:
			break;
		case Bin_Win:
			PORTB = SetBit(PORTB, 0, 0) & SetBit(PORTB, 1, 0) & SetBit(PORTB, 2, 0) & SetBit(PORTB, 3, 0) & SetBit(PORTB, 4, 0) & SetBit(PORTB, 5, 0) & SetBit(PORTB, 7, 0);
			PORTD = 0x00;
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
	IsActives[3] = 0;
	IsActives[4] = 0;
	IsActives[5] = 0;
	tasks[0].state = Start;
	tasks[2].state = Start_light;
	tasks[3].state = Start_Note;
	tasks[4].state = Start_fran;
	tasks[5].state = Start_hex;
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
	IsActives[3] = 0;
	IsActives[4] = 0;
	IsActives[5] = 0;
	tasks[0].state = Start;
	tasks[2].state = Start_light;
	tasks[3].state = Start_Note;
	tasks[4].state = Start_fran;
	tasks[5].state = Start_hex;
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
	if(GamePOINTER > 3)
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
	IsActives[3] = 0;
	IsActives[4] = 0;
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
	tasks[i].state = Start_hex;
	tasks[i].period = 1000;
	tasks[i].elapsedTime = 0;
	tasks[i].TickFct = &MemLightsTck;
	tasks[i].isActive = 0;
	i++;
	tasks[i].state = Start_Note;
	tasks[i].period = 1000;
	tasks[i].elapsedTime = 0;
	tasks[i].TickFct = &NotesTck;
	tasks[i].isActive = 0;
	i++;
	tasks[i].state = Start_fran;
	tasks[i].period = 1000;
	tasks[i].elapsedTime = 0;
	tasks[i].TickFct = &FranchiseTck;
	tasks[i].isActive = 0;
	i++;
	tasks[i].state = Start_light;
	tasks[i].period = 1000;
	tasks[i].elapsedTime = 0;
	tasks[i].TickFct = &Hex2BinTck;
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