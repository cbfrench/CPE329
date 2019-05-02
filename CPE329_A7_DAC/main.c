#include "msp.h"
#include "delay.h"

#define SHUTDOWN BIT4
#define GAIN BIT5
#define CS BIT0

#define LENGTH 60000

volatile uint16_t voltage = 0;
volatile int8_t factor = 1;

void init_SPI(void) {

    EUSCI_B0->CTLW0 |= EUSCI_B_CTLW0_SWRST;
    EUSCI_B0->CTLW0 = EUSCI_B_CTLW0_CKPL
                    | EUSCI_B_CTLW0_MSB
                    | EUSCI_B_CTLW0_MST
                    | EUSCI_B_CTLW0_SYNC
                    | EUSCI_B_CTLW0_UCSSEL_2
                    | EUSCI_B_CTLW0_SWRST;
    EUSCI_B0->BRW = 0x01;
    P1->SEL0 |= (BIT5|BIT6);
    P1->SEL1 &= ~(BIT5|BIT6);
    P1->DIR |= BIT0;
    P1->OUT |= CHIP_SEL;
    EUSCI_B0->CTLW0 &= ~EUSCI_B_CTLW0_SWRST;
}

void set_voltage(uint16_t val) {
    int loByte, hiByte;
    loByte = val & 0xFF;
    hiByte = (val >> 8) & 0x0F;
    hiByte |= (SHUTDOWN|GAIN);
    P1->OUT &= ~CHIP_SEL;
    while(!(EUSCI_B0->IFG & EUSCI_B_IFG_TXIFG));
    EUSCI_B0->TXBUF = hiByte;
    while(!(EUSCI_B0->IFG & EUSCI_B_IFG_TXIFG));
    EUSCI_B0->TXBUF = loByte;
    while(!(EUSCI_B0->IFG & EUSCI_B_IFG_RXIFG));
    P1->OUT |= CHIP_SEL;
}

void TA0_0_IRQHandler(void) {
    TIMER_A0->CCTL[0] &= ~TIMER_A_CCTLN_CCIFG;
    TIMER_A0->CCR[0] += LENGTH;
    factor *= -1;
}

void TA0_N_IRQHandler(void) {
    if (TIMER_A0->CCTL[1] & TIMER_A_CCTLN_CCIFG) {
        TIMER_A0->CCTL[1] &= ~TIMER_A_CCTLN_CCIFG;
        TIMER_A0->CCR[1] += 50;
        voltage += factor*2;
    }
}

void init_timers(){
    TIMER_A0->CTL |= TIMER_A_CTL_TASSEL_2 | TIMER_A_CTL_MC_2 | TIMER_A_CTL_ID_1;
    TIMER_A0->CCR[0] = LENGTH;
    TIMER_A0->CCR[1] = 25;
    TIMER_A0->CCTL[0] |= TIMER_A_CCTLN_CCIE;
    TIMER_A0->CCTL[1] |= TIMER_A_CCTLN_CCIE;
    NVIC->ISER[0] = 1 << (TA0_0_IRQn & 31);
    NVIC->ISER[0] = 1 << (TA0_N_IRQn & 31);
}

void main(void) {
    set_DCO(FREQ_12_MHz);
    init_timers();
    init_SPI();
    set_voltage(0);
    __enable_irq();
    while(1) {
        set_voltage(voltage);
    }
}
