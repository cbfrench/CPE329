/****************************main.c***********************/

#include "msp.h"
#include "delay.h"
#include "dac.h"
#include <string.h>

int adc_flag = 0;
volatile uint16_t readValue;
volatile double writeValue;

int flag = 0;
char inValue[5];
int index = 0;

/**
 * Outputs individual characters over UART
 */
void print_char(char letter) {
    while(!(EUSCI_A0->IFG & EUSCI_A_IFG_TXIFG));
    EUSCI_A0->TXBUF = letter;
}

/**
 * Outputs String Characters over UART
 */
void print_string(char* string) {
    int i;
    for (i = 0; i < strlen(string); i++)
        print_char(string[i]);
    print_char('\n');
}

/**
 * Function for processing characters recieved via UART
 */
void EUSCIA0_IRQHandler(void) {
    char letter;
    if (EUSCI_A0->IFG & EUSCI_A_IFG_RXIFG) {
        // Cancel Interrupt Flag
        EUSCI_A0->IFG &= ~EUSCI_A_IFG_RXIFG;

        // Process ASCII Character received by UART into a character array
        letter = EUSCI_A0->RXBUF;
        while(!(EUSCI_A0->IFG & EUSCI_A_IFG_TXIFG));
        EUSCI_A0->TXBUF = letter;
        if (letter >= '0' && letter <= '9' && index < 4) {
            inValue[index] = letter;
            index++;
        }
        if (letter == 0xD) {
            // If carrage return is detected, cancel read flag
            inValue[index] = '\0';
            flag = 1;
        }
        if(index == 4){
            inValue[index] = '\0';
        }
    }
}

/**
 * Initialize UART Communications
 */
void init_UART() {
    EUSCI_A0->CTLW0 |= EUSCI_A_CTLW0_SWRST;         // Enable Software Reset
    EUSCI_A0->CTLW0 =   EUSCI_A_CTLW0_PEN           // Parity (Odd)
                      | EUSCI_A_CTLW0_SPB           // 2 Stop Bits
                      | EUSCI_A_CTLW0_MODE_0        // UART mode
                      | EUSCI_A_CTLW0_SSEL__SMCLK   // Use SMCLK
                      | EUSCI_A_CTLW0_SWRST;        // Keep in Software Reset

    EUSCI_A0->BRW = 0x01;                           // From UCBRx Calculation
    EUSCI_A0->MCTLW = (10 << EUSCI_A_MCTLW_BRF_OFS) // Need to shift 4 bits
                      | EUSCI_A_MCTLW_OS16;         // Increase Sensitivity

    P1->SEL0 |= (BIT2 | BIT3);                      // Select EUSCI_A0 Tx and Rx
    P1->SEL1 &= ~(BIT2 | BIT3);

    EUSCI_A0->CTLW0 &= ~EUSCI_A_CTLW0_SWRST;        // Disable Software Reset

    EUSCI_A0->IFG &= ~EUSCI_A_IFG_RXIFG;            // Clear and Enable Rx Interrupt Flags
    EUSCI_A0->IE |= EUSCI_A_IE_RXIE;
}

void init_ADC(){
    ADC14->CTL0 &= ~ADC14_CTL0_ENC;                 //disable ADC
    ADC14->CTL0 = ADC14_CTL0_SHP                    //turn on, use SMCLK, set number of cycles to 16, set pulse mode to use ADC sample timer
            | ADC14_CTL0_SSEL_4
            | ADC14_CTL0_SHT1__16
            | ADC14_CTL0_ON;
    ADC14->CTL1 = (14 << ADC14_CTL1_CSTARTADD_OFS)  //start at mem 14
            | ADC14_CTL1_RES_3;
    ADC14->MCTL[14] = ADC14_MCTLN_INCH_14;          //select channel 14
    ADC14->IER0 |= ADC14_IER0_IE14;                 //enable interrupts on mem 14
    ADC14->CTL0 |= ADC14_CTL0_ENC;                  //enable ADC
}

void ADC14_IRQHandler(void){
    readValue = ADC14->MEM[14];                     //set read value
    adc_flag = 1;                                   //set flag
}

void ADC_4_cycles(){
    writeValue = 0.0002 * readValue + 0.0185;       //set output value for 4 cycles
}

void ADC_16_cycles(){
    writeValue = 0.0002 * readValue + 0.0001;       //set output value for 16 cycles
}

void ADC_96_cycles(){
    writeValue = 0.0002 * readValue + 0.0008;       //set output value for 96 cycles
}

void ADC_192_cycles(){
    writeValue = 0.0002 * readValue + 0.0024;       //set output value for 192 cycles
}

void print_float(float input){
    float f = input;
    int count = 0;
    int round;
    while(f != 0 && count < 5){                     //while float is not zero, truncate to 5 chars
        if(count == 1){
            print_char('.');                        //print decimal at position 1
        }
        else{
            round = (int)f;
            print_char(round + '0');
            f = (f - round) * 10;                   //get decimal and continue
        }
        count++;
    }
    print_char('\n');
}

void main(void)
{
	WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;		// stop watchdog timer
	set_DCO(FREQ_3_MHz);

	init_UART();
	init_ADC();

	NVIC->ISER[0] = (1 << (ADC14_IRQn & 31));
	NVIC->ISER[1] = (1 << (EUSCIA0_IRQn & 31));

	__enable_irq();

    P6->SEL0 |= BIT1;
    P6->SEL1 |= BIT1;

	ADC14->CTL0 |= ADC14_CTL0_SC;                   //start conversion
	while(1){
	    if(adc_flag){
	        adc_flag = 0;                           //reset flag
	        ADC_16_cycles();                        //use 16 cycles
	        print_float(writeValue);                //print converted value to UART
	        ADC14->CTL0 |= ADC14_CTL0_SC;           //start conversion
	    }
	    delay_us(100000);                           //delay
	}
}
