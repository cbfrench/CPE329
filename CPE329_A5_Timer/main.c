#include "msp.h"
#include "delay.h"

// CURRENTLY CONFIGURED FOR A5 PART C
#define LENGTH 5000000

#define OUTPUT P5

/**
 * main.c
 */

int counter = 0;

void TA0_0_IRQHandler(void){
    //OUTPUT->OUT |= BIT5;

    if(TIMER_A0->CCTL[0] & TIMER_A_CCTLN_CCIFG){
        TIMER_A0->CCTL[0] &= ~TIMER_A_CCTLN_CCIFG;

        //OUTPUT->OUT ^= BIT7;
        counter++;
        TIMER_A0->CCR[0] += LENGTH;
    }

    //OUTPUT->OUT &= ~BIT5;
}

void main(void)
{
	WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;		    // stop watchdog timer

	set_DCO(FREQ_1_5_MHz);

	// Set-Up P4.3 to output MCLK Signal
    /*P4 -> DIR |= BIT3;                                  // Direction: Output
    P4 -> SEL0 |= BIT3;                                 // Set for Primary Output
    P4 -> SEL1 &= BIT3;*/

    // Set up LED
    P1 -> SEL0 &= BIT0;
    P1 -> SEL1 &= BIT0;
    P1 -> DIR |= BIT0;

    // Set-Up P5.7 and P5.5 as GPIO Outputs
	/*OUTPUT->SEL0 &= ~(BIT7 | BIT5);                     // Set for GPIO
	OUTPUT->SEL1 &= ~(BIT7 | BIT5);
	OUTPUT->DIR |= BIT7 | BIT5;                         // Direction: Output
*/

	//set output to be measured on oscilloscope
	TIMER_A0->CTL |= TIMER_A_CTL_SSEL__SMCLK | TIMER_A_CTL_MC_2 | TIMER_A_CTL_ID_0;

	TIMER_A0->CCR[0] = LENGTH;                          // Set CCR0 and CCR1 lengths

	TIMER_A0->CCTL[0] |= TIMER_A_CCTLN_CCIE;            // Enable Interrupts
	NVIC->ISER[0] = 1 << (TA0_0_IRQn & 31);
	__enable_irq();
	while(1){
	    if(counter >= 750){
	        printf("BLINK\n");
	        P1->OUT ^= BIT0;
	        counter = 0;
	    }
	}
}
