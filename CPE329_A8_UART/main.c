#include "msp.h"
#include "delay.h"
#include "dac.h"


void print_char(char letter);
void print_string(char* string);

int flag = 0;
char inValue[5];
int index = 0;

void print_char(char letter) {
    while(!(EUSCI_A0->IFG & EUSCI_A_IFG_TXIFG));
    EUSCI_A0->TXBUF = letter;
}

void print_string(char* string) {
    int i;
    for (i = 0; i < strlen(string); i++)
        print_char(string[i]);
}

void EUSCIA0_IRQHandler(void) {
    char letter;
    if (EUSCI_A0->IFG & EUSCI_A_IFG_RXIFG) {
        letter = EUSCI_A0->RXBUF;
        while(!(EUSCI_A0->IFG & EUSCI_A_IFG_TXIFG));
        EUSCI_A0->TXBUF = letter;
        if (letter >= '0' && letter <= '9' && index < 4) {
            inValue[index] = letter;
            index++;
        }
        if (letter == 0xD) {
            inValue[index] = '\0';
            flag = 1;
        }
    }
}

void init_UART(){
    EUSCI_A0->CTLW0 |= EUSCI_A_CTLW0_SWRST;
    EUSCI_A0->CTLW0 = EUSCI_A_CTLW0_PEN
            | EUSCI_A_CTLW0_SPB
            | EUSCI_A_CTLW0_UCSSEL_2
            | EUSCI_A_CTLW0_MODE_0
            | EUSCI_A_CTLW0_SWRST;
    EUSCI_A0->BRW = 0x01;
    EUSCI_A0->MCTLW = (10 << EUSCI_A_MCTLW_BRF_OFS)
            | EUSCI_A_MCTLW_OS16;
    EUSCI_A0->CTLW0 &= ~EUSCI_A_CTLW0_SWRST;
    EUSCI_A0->IE |= EUSCI_A_IE_RXIE;
    NVIC->ISER[0] = 1 << (EUSCIA0_IRQn & 31);
    P1->SEL0 |= (BIT2|BIT3);
    P1->SEL1 &= ~(BIT2|BIT3);
    __enable_irq();
}

void main(void)
{
    int voltage = 0;
    WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;     // stop watchdog timer
    set_DCO(FREQ_3_MHz);
    init_SPI();
    init_UART();
    while(1) {
        if (flag) {
            print_string(inValue);
            voltage = atoi(inValue);
            if (voltage < 4096 && voltage >= 0)
                set_voltage(voltage);
            flag = 0;
            index = 0;
            voltage = 0;
        }
    }
}
