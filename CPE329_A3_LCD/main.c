#include "msp.h"
#include "delay.h"
#include "lcd.h"

/**
 * main.c
 */
void main(void)
{
	WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;		// stop watchdog timer

	P4 -> SEL0 &= ~ (BIT0 | BIT1 | BIT2 | BIT3);
	P4 -> SEL1 &= ~(BIT0 | BIT1 | BIT2 | BIT3);
	P4 -> DIR |= BIT0 | BIT1 | BIT2 | BIT3;

	P3 -> SEL0 &= ~(BIT5 | BIT6 | BIT7);
	P3 -> SEL1 &= ~(BIT5 | BIT6 | BIT7);
	P3 -> DIR |= BIT5 | BIT6 | BIT7;


	initLCD();
	changeLine(1);
	writeStringLCD("Hello World");
	changeLine(2);
	writeStringLCD("Assignment 3");

}
