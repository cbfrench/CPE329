#include "msp.h"
#include "delay.h"
#include "lcd.h"
#include "keypad.h"

/**
 * main.c
 */
void main(void)
{
    int keyPressed;
	WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;

	//set up RS, RW, E for LCD
	P3 -> SEL0 &= ~(RS | RW | E);
	P3 -> SEL1 &= ~(RS | RW | E);
	P3 -> DIR |= RS | RW | E;

	//set up data for LCD
	P4 -> SEL0 &= ~ (D04 | D05 | D06 | D07);
	P4 -> SEL1 &= ~(D04 | D05 | D06 | D07);
	P4 -> DIR |= D04 | D05 | D06 | D07;

	//set up pins for keypad rows
	P2 -> SEL0 &= ~ (ROW1 | ROW2 | ROW3 | ROW4);
	P2 -> SEL1 &= ~(ROW1 | ROW2 | ROW3 | ROW4);
	P2 -> DIR |= ROW1 | ROW2 | ROW3 | ROW4;
	P2 -> REN |= ROW1 | ROW2 | ROW3 | ROW4;
	P2 -> OUT &= ~(ROW1 | ROW2 | ROW3 | ROW4);

	//set up pins for keypad columns
	P5 -> SEL0 &= ~ (COL1 | COL2 | COL3);
	P5 -> SEL1 &= ~(COL1 | COL2 | COL3);
	P5 -> DIR |= COL1 | COL2 | COL3;

	init_LCD();
	while(1){
	    keyPressed = check_key_pressed();
	    if(keyPressed != -1){
	        clear_LCD();
	        home_LCD();
	        write_char_LCD(keyPressed);
	    }
	}
}
