
#include <avr/io.h>
#include <avr/sleep.h>
#include "io.c" // Our io.c file from lab 5
#include <avr/pgmspace.h> // Built in avr library
#include "usart.h"
#include <avr/interrupt.h>
#include <stdio.h>
#include <avr/eeprom.h>

// This is how we define a binary array of our character pattern.
// Each const represents a 5x8 pixel grid, with ONES representing
// illuminated pixels, and ZEROS representing non-illuminated pixels.



unsigned long _avr_timer_M = 1; // Start count from here, down to 0. Default 1 ms.
unsigned long _avr_timer_cntcurr = 0; // Current internal count of 1ms ticks

void TimerOn() {
	// AVR timer/counter controller register TCCR1
	TCCR1B = 0x0B;// bit3 = 0: CTC mode (clear timer on compare)
	// bit2bit1bit0=011: pre-scaler /64
	// 00001011: 0x0B
	// SO, 8 MHz clock or 8,000,000 /64 = 125,000 ticks/s
	// Thus, TCNT1 register will count at 125,000 ticks/s

	// AVR output compare register OCR1A.
	OCR1A = 125;	// Timer interrupt will be generated when TCNT1==OCR1A
	// We want a 1 ms tick. 0.001 s * 125,000 ticks/s = 125
	// So when TCNT1 register equals 125,
	// 1 ms has passed. Thus, we compare to 125.
	// AVR timer interrupt mask register
	TIMSK1 = 0x02; // bit1: OCIE1A -- enables compare match interrupt

	//Initialize avr counter
	TCNT1=0;

	_avr_timer_cntcurr = _avr_timer_M;
	// TimerISR will be called every _avr_timer_cntcurr milliseconds

	//Enable global interrupts
	SREG |= 0x80; // 0x80: 1000000
}

void TimerOff() {
	TCCR1B = 0x00; // bit3bit1bit0=000: timer off
}

// In our approach, the C programmer does not touch this ISR, but rather TimerISR()
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



unsigned char SetBit(unsigned char x, unsigned char k, unsigned char b) {
	return (b ? x | (0x01 << k) : x & ~(0x01 << k));
}
unsigned char GetBit(unsigned char x, unsigned char k) {
	return ((x & (0x01 << k)) != 0);
}


typedef struct task {
	unsigned long elapsed_time;
	unsigned int state;
	unsigned long period;
	int (*TickFunction)(int);
} task;

const unsigned char GCDperiod = 50;	//period of 1ms
const unsigned char number_of_tasks = 3;	//so far, 5 tasks completed.

task tasks[5];


void TimerISR() {
	unsigned char k = 0;

	for( k = 0; k < 5; k++ ) {
		if( tasks[k].elapsed_time > tasks[k].period ) {
			tasks[k].state = tasks[k].TickFunction(tasks[k].state);
			tasks[k].elapsed_time = 0;
				
		}
		tasks[k].elapsed_time += GCDperiod;
	}
}






uint8_t score = 0x00;
uint8_t high_score = 0x00;


const uint8_t pixel_man[] PROGMEM = {
	0b00100,
	0b01010,
	0b11111,
	0b10101,
	0b00100,
	0b01010,
	0b01010,
	0b11011
	
}; // biped that kinda looks like Buzz Lightyear on an Atari 2600

const uint8_t dash_waves[] PROGMEM = {
	0b1010,
	0b0101,
	0b0000,
	0b1010,
	0b0101,
	0b0000,
	0b1010,
	0b0101,
	
};

const uint8_t space[] PROGMEM = {
	0b0000,
	0b0000,
	0b0000,
	0b0000,
	0b0000,
	0b0000,
	0b0000,
	0b0000,
	
};


// The following function can be found in the lcd_lib.c file found on the accompanying website
// The only change I made was the function names to our pre-built functions (LCD_WriteCommand and LCD_WriteData)
// as they have the same functionality.
void LCDdefinechar(const uint8_t *pc,uint8_t char_code){
	uint8_t a, pcc;
	uint16_t i;
	a=(char_code<<3)|0x40;
	for (i=0; i<8; i++){
		pcc=pgm_read_byte(&pc[i]);
		LCD_WriteCommand(a++);
		LCD_WriteData(pcc);
	}
}



enum pixel_man_states { move_left, move_right } pixel_man_state;
unsigned char pixel_pos = 9;
int move_pixel_man( int pixel_man_state) {
	switch( pixel_man_state ) {
		case move_right:
			if( pixel_pos > 13 ) {
				pixel_man_state = move_left;
			}
			break;
		
		case move_left:
			if( pixel_pos < 10 ) {
				pixel_man_state = move_right;
			}
			break;
			
		default:
			pixel_man_state = move_right;
			break;
	}
	
	switch( pixel_man_state ) {
		case move_right:
			pixel_pos++;
			
			LCDdefinechar(space,3);
			LCD_Cursor(pixel_pos-1);
			LCD_WriteData(0b00000011);
		
			LCDdefinechar(dash_waves,1);
			LCD_Cursor(pixel_pos);
			LCD_WriteData(0b00000001);
			
			LCDdefinechar(pixel_man,2);
			LCD_Cursor(pixel_pos+1);
			LCD_WriteData(0b00000010);
			
			break;
		
		case move_left:
			pixel_pos--;
			
			LCDdefinechar(space,3);
			LCD_Cursor(pixel_pos+2);
			LCD_WriteData(0b00000011);
			
			LCDdefinechar(pixel_man,2);
			LCD_Cursor(pixel_pos);
			LCD_WriteData(0b00000010);
			
			LCDdefinechar(dash_waves,1);
			LCD_Cursor(pixel_pos+1);
			LCD_WriteData(0b00000001);
			break;
		
		default:
			break;
	}
	
	return pixel_man_state;
}



void Display_score() {
	LCD_Cursor(17);
	LCD_WriteData('H');
	LCD_Cursor(18);
	LCD_WriteData('S');
	LCD_Cursor(19);
	LCD_WriteData('C');
	LCD_Cursor(20);
	LCD_WriteData('O');
	LCD_Cursor(21);
	LCD_WriteData('R');
	LCD_Cursor(22);
	LCD_WriteData('E');
	LCD_Cursor(23);
	LCD_WriteData(':');
	LCD_Cursor(28);
	LCD_WriteData('/');
}




enum receive_score_states { has_received, receive, flush } receive_score_state;
int receive_score( int state ) {
	
	switch( state ) {
			
		case has_received:
			if( USART_HasReceived(0) ) {
				state = receive;
			} else {
				state = has_received;
			}
			break;
			
		case receive:
			state = flush;
			break;
			
		case flush:
			state = has_received;;
			break;
			
		default:
			state = has_received;
			break;
		
	}
	
	switch( state ) {
		
		case has_received:
			break;
		
		case receive:
			score = USART_Receive(0);
			break;
		
		case flush:
			USART_Flush(0);
			break;
		
		default:
			break;
		
	}
	
	return state;
}


void write_high_score() {
	
	if( eeprom_is_ready() ) {
		_EEPUT( (uint8_t*)1, high_score );
	}

}

unsigned char read_high_score() {
	
	unsigned char score_read;
	
	if( eeprom_is_ready() ){
		_EEGET(score_read, (const uint8_t*)1 );
	}
	
	return score_read;
	
}


void print_scores() {
	
	high_score = read_high_score();
	
	unsigned char tmp_high_score = high_score;
	unsigned char tmp_score = score;
	
	//print highest score
	
	LCD_Cursor(25);
	LCD_WriteData( tmp_high_score / 100 + '0');
	tmp_high_score = tmp_high_score - (tmp_high_score / 100) * 100;
	
	LCD_Cursor(26);
	LCD_WriteData(tmp_high_score / 10 + '0');
	
	LCD_Cursor(27);
	LCD_WriteData(tmp_high_score % 10 + '0');
	
	//print current score
	
	LCD_Cursor(30);
	LCD_WriteData( tmp_score / 100  + '0');
	tmp_score = tmp_score - (tmp_score / 100) * 100;
	
	LCD_Cursor(31);
	LCD_WriteData(tmp_score / 10 + '0');
	
	LCD_Cursor(32);
	LCD_WriteData(tmp_score % 10 + '0');
	
}





enum clear_hs_states { button_off, button_on, button_release } clear_hs_state;
void clear_high_score() {
	
	unsigned char tmpC = PORTC;
	
	switch( clear_hs_state ) {
		case button_off:
			if( GetBit(~tmpC,7) ) {
				clear_hs_state = button_on;
			} else {
				clear_hs_state = button_off;
			}
			break;
		
		case button_on:
			clear_hs_state = button_release;
			break;

		case button_release:
			if( !GetBit(~tmpC,7) ) {
				clear_hs_state = button_off;
			} else {
				clear_hs_state = button_release;
			}
			break;
			
		default:
			clear_hs_state = button_off;
			break;
	}
	
	switch( clear_hs_state ) {
		case button_off:
			break;
		
		case button_on:
			//_EEPUT( (uint8_t*)1, 0 );
			high_score = 0;
			write_high_score();
			break;
			
		case button_release:
			break;
		
		default:
			break;
	}
	
}



enum high_score_states { lessthan, greaterthan, wait_eeprom } high_score_state;
int check_high_score( int high_score_state ) {
	
	high_score = read_high_score();
	
	switch( high_score_state ) {
		
		case lessthan:
			if( score < high_score ) {
				high_score_state = lessthan;
			} else {
				/*high_score_state = wait_eeprom;*/
				high_score_state = greaterthan;
			}
			break;
			
// 		case wait_eeprom:
// 			if( eeprom_is_ready() ) {
// 				high_score_state = greaterthan;
// 			} else {
// 				high_score_state = wait_eeprom;
// 			}
// 			break;
			
		case greaterthan:
			high_score_state = lessthan;
			break;
			
		default:
			high_score_state = lessthan;
			break;
	}
	
	switch( high_score_state ) {
		
		case lessthan:
			break;
		
		case greaterthan:
			high_score = score;
			write_high_score();
			break;
		
		default:
			break;
	}
	
	return high_score_state;
	
}




int main(void)
{
	TimerOn();
	TimerSet(GCDperiod);
	
	initUSART(0);
	USART_Flush(0);
	
	DDRA = 0xFF;	PORTA = 0x00;
	DDRC = 0x7F;	PORTC = 0x80;
	DDRB = 0xFF;	PORTB = 0x00;
	DDRD = 0x02;	PORTD = 0xFD;
	
 
	
	LCD_init();
	LCD_ClearScreen();
	// Now I know how to freely access individual characters in LCD memory.
	// Need to define my own characters.
	
	// The following calls to LCDdefinechar define a special character in one of the
	// EIGHT AVAILABLE (0 - 7) spaces in the LCD memory.
	// Note that you can only have up to eight unique characters stored 
	// in LCD memory at any given time.
	// The binary address passed into LCD_WriteData was determined using the chart 
	// on slide 4 found by following the CS 3710 url
   
    
	
	unsigned char i = 0;
	
	tasks[i].state = move_right;
	tasks[i].period = 300;
	tasks[i].elapsed_time = 0;
	tasks[i].TickFunction = &move_pixel_man;
	i++;
	
	tasks[i].state = 0;
	tasks[i].period = 100;
	tasks[i].elapsed_time = 0;
	tasks[i].TickFunction = &print_scores;
	i++;
	
	tasks[i].state = has_received;
	tasks[i].period = 100;
	tasks[i].elapsed_time = 0;
	tasks[i].TickFunction = &receive_score;
	i++;
	
	tasks[i].state = lessthan;
	tasks[i].period = 100;
	tasks[i].elapsed_time = 0;
	tasks[i].TickFunction = &check_high_score;
	i++;

	
	tasks[i].state = 0;
	tasks[i].period = 100;
	tasks[i].elapsed_time = 0;
	tasks[i].TickFunction = &clear_high_score;
	i++;
	
	
	LCD_DisplayString(1, "DODGER!");
	
	Display_score();
	
	high_score = read_high_score();
	
    while(1) {	
		sleep_mode();
	}
}

