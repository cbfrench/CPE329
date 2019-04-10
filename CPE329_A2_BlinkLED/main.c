#include "msp.h"
#include "delay.h"

void set_DCO(uint32_t frequency)
{
    if(frequency == FREQ_48_MHz)
    {
        // Adjust PCM to allow greater power use when using 48 MHz
        while((PCM->CTL1 & PCM_CTL1_PMR_BUSY));
        PCM->CTL0 = PCM_CTL0_KEY_VAL | PCM_CTL0_AMR_1;
        while((PCM->CTL1 & PCM_CTL1_PMR_BUSY));

        FLCTL->BANK0_RDCTL = (FLCTL->BANK0_RDCTL &
         ~(FLCTL_BANK0_RDCTL_WAIT_MASK)) | FLCTL_BANK0_RDCTL_WAIT_1;
        FLCTL->BANK1_RDCTL = (FLCTL->BANK0_RDCTL &
         ~(FLCTL_BANK1_RDCTL_WAIT_MASK)) | FLCTL_BANK1_RDCTL_WAIT_1;
    }

    // Use DCO as source for MCLK and set DCO Frequency from 1.5 to 48MHz
    CS -> KEY = CS_KEY_VAL;                             // Unlock CS

    CS -> CTL0 = CS_CTL0_DCOEN | frequency;             // Enable DCOCLK | Set Freq
    CS -> CTL1 = CS_CTL1_DIVM_0 | CS_CTL1_SELM__DCOCLK; // Divide by 1 | Use DCOCLK

    CS -> CLKEN = CS_CLKEN_MCLK_EN | CS_CLKEN_REFOFSEL; // Enable MCLK | Enable Nominal Ref

    CS -> KEY = 0;                                      // Lock CS
}

void delay_uS(int delay)
{
    long i;

    int multiplier_1_5_MHz = (delay > 1000) ? 2 : 1; //0.008 ideal
    int multiplier_3_MHz   = (delay > 1000) ? 4 : 3;//8;
    int multiplier_6_MHz   = 7;//606;
    int multiplier_12_MHz  = 1;//1212;
    int multiplier_24_MHz  = 1;//2424;
    int multiplier_48_MHz  = 1;//4412;

    long delayLoopIterator;

    switch((CS -> CTL0)&CS_CTL0_DCORSEL_MASK)
    {
        case FREQ_48_MHz:
            delayLoopIterator = (((long)delay/10)*multiplier_48_MHz);    // 48 MHz
            break;
        case FREQ_24_MHz:
            delayLoopIterator = (((long)delay/10)*multiplier_24_MHz);    // 24 MHz
            break;
        case FREQ_12_MHz:
            delayLoopIterator = (((long)delay/10)*multiplier_12_MHz);    // 12 MHz
            break;
        case FREQ_6_MHz:
            delayLoopIterator = (((long)delay/10)*multiplier_6_MHz);    // 6 MHz
            break;
        case FREQ_3_MHz:
            delayLoopIterator = (((long)delay/10)*multiplier_3_MHz); // 3 MHz
            break;
        default:
            delayLoopIterator = (((long)delay/10)*multiplier_1_5_MHz); // 1.5 MHz or all Others
            break;
    }

    for(i=delayLoopIterator; i > 0; i--);
}

/**
 * main.c
 */
void main(void)

{
	WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;		    // stop watchdog timer

	// Set-Up P4.3 to output MCLK Signal
    P4 -> DIR |= BIT3;                                  // Direction: Output
    P4 -> DS &= BIT3;                                   // Regular Output Power
    P4 -> SEL0 |= BIT3;                                 // Set for Primary Output
    P4 -> SEL1 &= BIT3;

    // Set-Up P4.1 as an Output
    P4 -> DIR |= BIT1;                                  // Direction: Output
    P4 -> DS &= BIT1;                                   // Regular Output Power
    P4 -> SEL1 &= BIT1;                                 // Set as GPIO
    P4 -> SEL0 &= BIT1;

    set_DCO(FREQ_6_MHz);

	while(1)
	{
	    delay_uS(10000);

        P4 -> OUT ^= BIT1;
	}

}















