#include "msp.h"
#include "delay.h"
#include "dac.h"

void init_ADC(){
    P6->SEL0 |= BIT1;
    P6->SEL1 |= BIT1;
    ADC14->CTL0 &= ~ADC14_CTL0_ENC;
    ADC14->CTL0 = ADC14_CTL0_SHP
            | ADC14_CTL0_SSEL_2
            | ADC14_CTL0_ON;
    ADC14->CTL1 = (0 << ADC14_CTL1_CSTARTADD_OFS)
            | ADC14_CTL1_RES_3;
    ADC14->MCTL[0] = ADC14_MCTLN_INCH_0;
    ADC14->IER0 = ADC14_IER0_IE0;
    ADC14->CTL0 |= ADC14_CTL0_ENC;
}

void ADC14_IRQHandler(void){
    volatile uint8_t readValue;
    readValue = ADC14->MEM[0];
}

void main(void)
{
	WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;		// stop watchdog timer
	set_DCO(FREQ_24_MHz);

	init_ADC();
	NVIC->ISER[0] = (1 << ADC14_IRQn & 31);
	__enable_irq();
	while(1){
	    ADC14->CTL0 |= ADC14_CTL0_SC;
	    delay_us(1000);
	}
}
