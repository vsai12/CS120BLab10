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

enum SM1_States { SM1_Press, SM1_Start, SM1_Enter, SM1_WaitRelease };
unsigned char combination[5] = {'1', '2', '3', '4', '5'};
int SM1_Tick(int state) {
	static unsigned char i = 0;
	static unsigned char key = '\0';
	key = GetKeypadKey();
	switch(state) {
		case SM1_Press:
			if(key == '#')
				state = SM1_Start;
			else
		       		state = SM1_Press;
			break;
		case SM1_Start:
			 if(key == '\0') {
				i = 0;
                                state = SM1_Enter;
			 }
                        else
                                state = SM1_Start;
			break;
		case SM1_Enter:	
			if(key == '#')
                                state = SM1_Start;
			else if(key == '\0') {
				state = SM1_Enter;
			}
                        else {
				if(key == combination[i]) {
					i++;
					state = SM1_WaitRelease;
				}
				else
					state = SM1_Press;
			}
			break;
		case SM1_WaitRelease:
			if(key == '\0') {
				if(i >= 5) {
					state = SM1_Press;
					SM1_output = 0x01;
				}
				else
                                	state = SM1_Enter;
                         }
                        else
                                state = SM1_WaitRelease;
                        break;

		default: state = SM1_Press; break;
	}
	return state;
}

enum SM2_States { SM2_Wait, SM2_Press };
int SM2_Tick(int state) {
	unsigned char input = ~PINB & 0x80;
	switch(state) {
		case SM2_Wait: 
			if(input == 0x80)
				state = SM2_Press;
			else
				state = SM2_Wait;
			break;
		case SM2_Press:
			if(input == 0x80)
				state = SM2_Press;
			else
				state = SM2_Wait;
			break;
		default: state = SM2_Wait; break;
	}

	switch(state) {
		case SM2_Wait:
			break;
		case SM2_Press:
			SM1_output = 0;
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
	DDRB = 0x7F; PORTB = 0x80;
	DDRC = 0xF0; PORTC = 0x0F;
	static task task1, task2, task3;
	task *tasks[] = { &task1, &task2, &task3 };
	const unsigned short numTasks = sizeof(tasks)/sizeof(task*);

	const char start = -1;

	task1.state = start;
	task1.period = 50;
	task1.elapsedTime = task1.period;
	task1.TickFct = &SM1_Tick;

	task2.state = start;
        task2.period = 50;
        task2.elapsedTime = task2.period;
        task2.TickFct = &SM2_Tick;

	task3.state = start;
	task3.period = 10;
	task3.elapsedTime = task3.period;
	task3.TickFct = &displaySMTick;

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
