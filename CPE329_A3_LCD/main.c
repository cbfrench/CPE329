#include "msp.h"
#include "delay.h"
#include "lcd.h"

/**
 * main.c
 */
void main(void)
{
	WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;		// stop watchdog timer
	P4 -> DIR |= BIT0 | BIT1 | BIT2 | BIT3;                                  // Direction: Output
	P4 -> DS &= BIT0 | BIT1 | BIT2 | BIT3;                                   // Regular Output Power
	P4 -> SEL0 &= BIT0 | BIT1 | BIT2 | BIT3;                                 // Set for Primary Output
	P4 -> SEL1 &= BIT0 | BIT1 | BIT2 | BIT3;
	P3 -> DIR |= BIT5 | BIT6 | BIT7;                                  // Direction: Output
	P3 -> DS &= BIT5 | BIT6 | BIT7;                                   // Regular Output Power
	P3 -> SEL0 &= BIT5 | BIT6 | BIT7;                                 // Set for Primary Output
	P3 -> SEL1 &= BIT5 | BIT6 | BIT7;

	set_DCO(FREQ_1_5_MHz);

	initLCD();
	writeStringLCD("Hello World");
	changeLine();
	writeStringLCD("Assignment 3");
	while(1){

	}
}
