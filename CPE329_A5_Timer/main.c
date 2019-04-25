#include "msp.h"
#include "delay.h"


#define LENGTH 3//960
#define SHORT_LENGTH 1//240

#define OUTPUT P5

/**
 * main.c
 */

void TA0_0_IRQHandler(void){
    TIMER_A0->CCTL[0] &= ~TIMER_A_CCTLN_CCIFG;
    OUTPUT->OUT ^= BIT7;
    TIMER_A0->CCR[0] += SHORT_LENGTH;
}

void TA0_N_IRQHandler(void){
    if(TIMER_A0->CCTL[1] & TIMER_A_CCTLN_CCIFG){
        TIMER_A0->CCTL[1] &= ~TIMER_A_CCTLN_CCIFG;
        OUTPUT->OUT ^= BIT7;
        TIMER_A0->CCR[1] += LENGTH;
    }
}

void main(void)
{
	WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;		// stop watchdog timer

	set_DCO(FREQ_24_MHz);

	// Set-Up P4.3 to output MCLK Signal
    P4 -> DIR |= BIT3;                                  // Direction: Output
    P4 -> DS &= BIT3;                                   // Regular Output Power
    P4 -> SEL0 |= BIT3;                                 // Set for Primary Output
    P4 -> SEL1 &= BIT3;


	OUTPUT->SEL0 &= ~(BIT7);
	OUTPUT->SEL1 &= ~(BIT7);
	OUTPUT->DIR |= BIT7;

	//set output to be measured on oscilloscope
	TIMER_A0->CTL |= TIMER_A_CTL_SSEL__SMCLK        // Use SMCLK
	              | TIMER_A_CTL_MC_2;               // Use Continuous Mode

	TIMER_A0->CCR[0] = SHORT_LENGTH;                // Set CCR0 and CCR1 lengths
	TIMER_A0->CCR[1] = LENGTH;

	TIMER_A0->CCTL[0] |= TIMER_A_CCTLN_CCIE;        // Enable Interrupts
	TIMER_A0->CCTL[1] |= TIMER_A_CCTLN_CCIE;
	NVIC->ISER[0] = 1 << (TA0_0_IRQn & 31);
	NVIC->ISER[0] = 1 << (TA0_N_IRQn & 31);
	__enable_irq();
	while(1);
}
