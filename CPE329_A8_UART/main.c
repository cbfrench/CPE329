#include <stdlib.h>
#include <string.h>
#include "msp.h"
#include "delay.h"
#include "dac.h"

int flag = 0;
char inValue[5];
int index = 0;

/**
 * Outputs individual characters over UART
 */
void print_char(char letter) {
    while(!(EUSCI_A0->IFG & EUSCI_A_IFG_TXIFG));
    EUSCI_A0->TXBUF = letter;

    P2->OUT ^=BIT2;
}

/**
 * Outputs String Characters over UART
 */
void print_string(char* string) {
    int i;
    for (i = 0; i < strlen(string); i++)
        print_char(string[i]);
}

/**
 * Function for processing characters recieved via UART
 */
void EUSCIA0_IRQHandler(void) {
    char letter;
    P2->OUT |= BIT2;
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
    }
    P2->OUT &= ~BIT2;

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

void main(void) {
    int voltage = 0;
    WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;     // stop watchdog timer
    set_DCO(FREQ_3_MHz);                            // Set Clock Speed
    init_timers();                                  // Initialize Timers, SPI, and UART
    init_SPI();
    init_UART();

    P2->SEL0 &= ~BIT2;                              // Set-Up Diagnostic LED
    P2->SEL1 &= ~BIT2;
    P2->DIR |= BIT2;

    NVIC->ISER[0] = 1 << (EUSCIA0_IRQn & 31);       // Enable Interrupts
    __enable_irq();

    while(1) {
        if (flag) {
            print_string(inValue);                  // Return Value sent over UART
            voltage = atoi(inValue);                // Convert sent value to Integer
            if (voltage < 4096 && voltage >= 0)
                set_voltage(voltage);               // Set Voltage if within allowed range
            flag = 0;                               // Clear Flags and temp. variables
            index = 0;
            voltage = 0;
        }
    }

}
