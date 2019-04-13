#include "msp.h"
#include "delay.h"
#include "lcd.h"

/**
 * main.c
 */
void main(void)
{
	WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;		// stop watchdog timer

	P4 -> SEL0 &= ~ (D04 | D05 | D06 | D07);        // Select Data Bits for use as GPIO
	P4 -> SEL1 &= ~(D04 | D05 | D06 | D07);
	P4 -> DIR |= D04 | D05 | D06 | D07;             // Set-up as Outputs

	P3 -> SEL0 &= ~(RS | RW | E);                   //Select Control Bits for use as GPIO
	P3 -> SEL1 &= ~(RS | RW | E);
	P3 -> DIR |= RS | RW | E;                       // Set-Up as Outputs

	set_DCO(FREQ_24_MHz);

	init_LCD();
	write_string("Hello World");
	change_line();
	write_string("Assignment 3");
	home_LCD();
}
