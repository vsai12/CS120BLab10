/*	Author: lab
 *  Partner(s) Name: 
 *	Lab Section:
 *	Assignment: Lab #  Exercise #
 *	Exercise Description: [optional - include for your own benefit]
 *
 *	I acknowledge all content contained herein, excluding template or example
 *	code, is my own original work.
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#ifdef _SIMULATE_
#include "simAVRHeader.h"
#endif

volatile unsigned char TimerFlag = 0;

unsigned long timer = 1;
unsigned long timer_count = 0;
void TimerOn() {
	TCCR1B = 0x0B;
	OCR1A = 125;
	TIMSK1 = 0x02;
	TCNT1 = 0;
	timer_count = timer;
	SREG |= 0x80;
}

void TimerOff() {
	TCCR1B = 0x00; 
}

void TimerISR() {
	TimerFlag = 1;
}

ISR(TIMER1_COMPA_vect) {
	timer_count--;
	if(timer_count == 0) {
		TimerISR();
		timer_count = timer;
	}
}

void TimerSet(unsigned long M) {
	timer = M;
	timer_count = timer;
}

unsigned char GetBit(unsigned char x, unsigned char k) {
   return ((x & (0x01 << k)) != 0);
}

unsigned char GetKeypadKey() {
	PORTC =  0xEF;
	asm("nop");
	if (GetBit(PINC,0) == 0) { return('1'); }
	if (GetBit(PINC,1) == 0) { return('4'); }
	if (GetBit(PINC,2) == 0) { return('7'); }
	if (GetBit(PINC,3) == 0) { return('*'); }

	PORTC = 0xDF;
	asm("nop");
	if (GetBit(PINC,0) == 0) { return('2'); }
        if (GetBit(PINC,1) == 0) { return('5'); }
        if (GetBit(PINC,2) == 0) { return('8'); }
        if (GetBit(PINC,3) == 0) { return('0'); }

	PORTC = 0xBF;
        asm("nop");
        if (GetBit(PINC,0) == 0) { return('3'); }
        if (GetBit(PINC,1) == 0) { return('6'); }
        if (GetBit(PINC,2) == 0) { return('9'); }
        if (GetBit(PINC,3) == 0) { return('#'); }

	PORTC = 0x0F;
        asm("nop");
        if (GetBit(PINC,0) == 0) { return('A'); }
        if (GetBit(PINC,1) == 0) { return('B'); }
        if (GetBit(PINC,2) == 0) { return('C'); }
        if (GetBit(PINC,3) == 0) { return('D'); }

	return('\0');

}

typedef struct task {
	signed char state;
	unsigned long int period;
	unsigned long int elapsedTime;
	int (*TickFct) (int);
} task;

unsigned char SM1_output = 0x00;

enum SM1_States { SM1_Press };

int SM1_Tick(int state) {
	switch(state) {
		case SM1_Press: state =  SM1_Press; break;
		default: state = SM1_Press; break;
	}

	switch(state) {
		case SM1_Press:
			switch (GetKeypadKey()) {
				case '\0': SM1_output = 0x1F; break; 
				case '1': SM1_output = 0x81; break;
				case '2': SM1_output = 0x82; break;
				case '3': SM1_output = 0x83; break;
				case '4': SM1_output = 0x84; break;
				case '5': SM1_output = 0x85; break;
				case '6': SM1_output = 0x86; break;
				case '7': SM1_output = 0x87; break;
				case '8': SM1_output = 0x88; break;
				case '9': SM1_output = 0x89; break;
				case 'A': SM1_output = 0x8A; break;
				case 'B': SM1_output = 0x8B; break;
				case 'C': SM1_output = 0x8C; break;
				case 'D': SM1_output = 0x8D; break;
				case '*': SM1_output = 0x8E; break;
				case '0': SM1_output = 0x80; break;
				case '#': SM1_output = 0x8F; break;
			}
			break;
	}
	return state;
}

enum display_States { display_display };

int displaySMTick(int state) {
	unsigned char output;
	switch(state) {
		case display_display: state = display_display; break;
		default: state = display_display; break;
	}

	switch(state) {
		case display_display:
			output = SM1_output;
			break;
	}
	PORTB = output;
	return state;
}

unsigned long int findGCD(unsigned long int a, unsigned long int b) {
	unsigned long int c;
	while(1) {
		c = a%b;
		if (c == 0) {return b;}
		a = b;
		b = c;
	}
	return 0;
}


int main(void) {
    /* Insert DDR and PORT initializations */
	DDRA = 0x00; PORTA = 0xFF;
	DDRB = 0xFF; PORTB = 0x00;
	DDRC = 0xF0; PORTC = 0x0F;
	static task task1, task2;
	task *tasks[] = { &task1, &task2 };
	const unsigned short numTasks = sizeof(tasks)/sizeof(task*);

	const char start = -1;

	task1.state = start;
	task1.period = 50;
	task1.elapsedTime = task1.period;
	task1.TickFct = &SM1_Tick;

	task2.state = start;
	task2.period = 10;
	task2.elapsedTime = task2.period;
	task2.TickFct = &displaySMTick;

	unsigned short i;
	unsigned long GCD = tasks[0]->period;
	for(i = 0; i < numTasks; i++) {
		GCD = findGCD(GCD, tasks[i]->period);
	}

	TimerSet(GCD);
	TimerOn();

    /* Insert your solution below */
    while (1) {
	for(i = 0; i < numTasks; i++) {
		if(tasks[i]->elapsedTime == tasks[i]->period) {
			tasks[i]->state = tasks[i]->TickFct(tasks[i]->state);
			tasks[i]->elapsedTime = 0;
		}
		tasks[i]->elapsedTime += GCD;
	}
	while(!TimerFlag);
	TimerFlag = 0;
    }
    return 0;
}
