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

void set_PWM(double frequency) {
	static double current_frequency;
	if(frequency != current_frequency) {
		if(!frequency) { TCCR3B &= 0x08; }
		else { TCCR3B |= 0x03; }

		if(frequency < 0.954) { OCR3A = 0xFFFF; }

		else if(frequency > 31250) { OCR3A = 0x0000; }

		else { OCR3A = (short)(8000000 / (128 * frequency)) - 1; }

		TCNT3 = 0;
		current_frequency = frequency;
	}
}

void PWM_on() {
	TCCR3A = (1 << COM3A0);
	TCCR3B = (1 << WGM32) | (1 << CS31) | (1 << CS30);
	set_PWM(0); //test if this works
}

void PWM_off() {
	TCCR3A = 0x00;
	TCCR3B = 0x00;
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

enum SM1_States { SM1_Press, SM1_Start, SM1_Enter, SM1_WaitRelease, SM1_Change, SM1_Input, SM1_WaitR2 };
unsigned char combination[5] = {'1', '2', '3', '4', '5'};
unsigned char newLock[4] = {'0'};
int SM1_Tick(int state) {
	static unsigned char i = 0;
	static unsigned char key = '\0';
	static unsigned char old = 1;
	static unsigned char max = 5;
	key = GetKeypadKey();
	unsigned char input = ~PINB & 0x80;
	switch(state) {
		case SM1_Press:
			if(key == '#')
				state = SM1_Start;
			else if(key == '*' && input == 0x80) 
				state = SM1_Change;
			else
		       		state = SM1_Press;
			break;
		case SM1_Change:
			if(key == '\0' && input == 0x00)
				state = SM1_Input;
			else {
				state = SM1_Change;
				max = 4;
				old = 0;
				i = 0;
			}
			break;
		case SM1_Input:
			if(key == '\0')
				state = SM1_Input;
			else {
				newLock[i] = key;
				i++;
				state = SM1_WaitR2;
			}
			break;
		case SM1_WaitR2:
			if(key == '\0') {
				if(i >= max)
					state = SM1_Press;
				else
					state= SM1_Input;
			}
			else 
				state = SM1_WaitR2;
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
				if(old) {
					if(key == combination[i]) {
						i++;
						state = SM1_WaitRelease;
					}
					else
						state = SM1_Press;
				}
				else {
					if(key == newLock[i]) {
                                                i++;
                                                state = SM1_WaitRelease;
                                        }
                                        else
                                                state = SM1_Press;
				}
			}
			break;
		case SM1_WaitRelease:
			if(key == '\0') {
				if(i >= max) {
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

unsigned char playing = 0;
void playMelody() {
        double freq[8] = {261.63, 293.66, 329.63, 349.23, 392, 440, 493.88, 523.25};
        unsigned int numNotes = 7;
        unsigned int melody[7] =   {0,1,2,0,2,02};
        unsigned int timeHeld[7] = {2,1,1,1,1,1,1};
        unsigned int timeBetween[6] = {1,0,0,0,0,0};
        static unsigned int count = 0;
        static unsigned int note = 0;
        static unsigned char playWait = 0;
        if(!playWait) {
                if(count < timeHeld[note]) {
                        set_PWM(freq[melody[note]]);
                        ++count;
                }
                else {
                        count = 0;
                        if(note < numNotes - 1) {
                                if(timeBetween[note] != 0) {
                                        playWait = 1;
                                }
                                else {
                                        ++note;
                                        set_PWM(freq[melody[note]]);
                                        ++count;
                                }
                        }
                        else {
                                playing = 0;
                                note = 0;
				set_PWM(0);
                        }
                }
        }
        if(playWait) {
                set_PWM(0);
                ++count;
                if(count == timeBetween[note]) {
                        playWait = 0;
                        ++note;
			count = 0;
                }
        }
}

enum SM3_States { SM3_Wait, SM3_On };
int SM3_Tick(int state) {
	unsigned char tmpA = ~PINA & 0x80;
	switch(state) {
		case SM3_Wait:
			if(tmpA) {
				state = SM3_On;
				playing = 1;
			}
			else
				state = SM3_Wait;
			break;
		case SM3_On:
			if(!tmpA && !playing)
				state = SM3_Wait;
			else
				state = SM3_On;
			break;
		default: state = SM3_Wait; break;
	}
	switch(state) {
		case SM3_Wait:
			break;
		case SM3_On:
			if(playing)
				playMelody();
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
	static task task1, task2, task3, task4;
	task *tasks[] = { &task1, &task2, &task3, &task4 };
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
        task3.period = 200;
        task3.elapsedTime = task3.period;
        task3.TickFct = &SM3_Tick;

	task4.state = start;
	task4.period = 10;
	task4.elapsedTime = task4.period;
	task4.TickFct = &displaySMTick;

	unsigned short i;
	unsigned long GCD = tasks[0]->period;
	for(i = 0; i < numTasks; i++) {
		GCD = findGCD(GCD, tasks[i]->period);
	}

	TimerSet(GCD);
	TimerOn();
	PWM_on();
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
