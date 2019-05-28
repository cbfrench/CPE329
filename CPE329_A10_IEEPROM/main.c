//******************************************************************************
// CPE 329 - Assignment 9
//
// Description: This demo connects an MSP432 to a Microchip 24LC256 EEPROM via
// the I2C bus. The MSP432 acts as the master and the EEPROM is a slave.
// The EEPROM uses 3 external connections A2 A1 A0 to set the lower 3 bits of
// its bus address. This creates a bus address of "1 0 1 0 A2 A1 A0". The code
// below assumes those three connections are all connected to VSS (Ground) and
// are logic 0. This gives the EEPROM a bus address of 0x50.
//
//
// /|\ /|\
// MSP432P401 10k 10k 24LC256 EEPROM
// master | | Slave
// ----------------- | | -----------------
// | P1.6/UCB0SDA|<-|----|->|SDA (5) |
// | | | | |
// | | | | |
// | P1.7/UCB0SCL|<-|------>|SCL (6) |
// | | | |
//
// Paul Hummel
// Cal Poly
// May 2017 (created)
// Built with CCSv8.1
//******************************************************************************
#include "msp.h"
#include <stdint.h>

#define EEPROM_ADDRESS 0x50

void InitEEPROM(uint8_t DeviceAddress);
void WriteEEPROM(uint16_t MemAddress, uint8_t MemByte);

uint8_t ReadEEPROM(uint16_t MemAddress);
uint16_t TransmitFlag = 0;

int main(void){
    uint32_t i;
    uint8_t value;
    WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD; // Stop watchdog timer
    P2->DIR |= BIT2 | BIT1 | BIT0; // Configure LED2
    P2->OUT &= ~(BIT2 | BIT1 | BIT0);
    __enable_irq(); // Enable global interrupt
    InitEEPROM(EEPROM_ADDRESS);
    WriteEEPROM(0x1122, 0x21);
    for (i = 4000; i > 0; i--); // Delay for EEPROM write cycle (5 ms)
    value = ReadEEPROM(0x1122); // Read value from EEPROM
    P2->OUT |= (value & (BIT2 | BIT1 | BIT0)); // Set LED2 with 3 LSB of value
    __sleep(); // go to lower power mode
}

/*
/ Initialize I2C bus for communicating with EEPROM.
*/
void InitEEPROM(uint8_t DeviceAddress){
    P1->SEL0 |= BIT6 | BIT7; // Set I2C pins of eUSCI_B0
    // Enable eUSCIB0 interrupt in NVIC module
    NVIC->ISER[0] = 1 << ((EUSCIB0_IRQn) & 31);
    // Configure USCI_B0 for I2C mode
    EUSCI_B0->CTLW0 |= EUSCI_A_CTLW0_SWRST; // Software reset enabled
    EUSCI_B0->CTLW0 = EUSCI_A_CTLW0_SWRST | // Remain eUSCI in reset mode
    EUSCI_B_CTLW0_MODE_3 | // I2C mode
    EUSCI_B_CTLW0_MST | // Master mode
    EUSCI_B_CTLW0_SYNC | // Sync mode
    EUSCI_B_CTLW0_SSEL__SMCLK; // SMCLK
    EUSCI_B0->BRW = 30; // baudrate = SMCLK / 30 = 100kHz
    EUSCI_B0->I2CSA = DeviceAddress; // Slave address
    EUSCI_B0->CTLW0 &= ~EUSCI_A_CTLW0_SWRST; // Release eUSCI from reset
    EUSCI_B0->IE |= EUSCI_A_IE_RXIE | // Enable receive interrupt
    EUSCI_A_IE_TXIE;
}

/*
/ Function that writes a single byte to the EEPROM.
/
/ MemAddress - 2 byte address specifies the address in the EEPROM memory
/ MemByte - 1 byte value that is stored in the EEPROM
/
/ Procedure :
/ start
/ transmit address+W (control+0) -> ACK (from EEPROM)
/ transmit data (high address) -> ACK (from EEPROM)
/ transmit data (low address) -> ACK (from EEPROM)
/ transmit data (data) -> ACK (from EEPROM)
/ stop
*/
void WriteEEPROM(uint16_t MemAddress, uint8_t MemByte){
    uint8_t HiAddress;
    uint8_t LoAddress;
    HiAddress = MemAddress >> 8;
    LoAddress = MemAddress & 0xFF;
    EUSCI_B0->CTLW0 |= EUSCI_B_CTLW0_TR; // Set transmit mode (write)
    EUSCI_B0->CTLW0 |= EUSCI_B_CTLW0_TXSTT; // I2C start condition
    while (!TransmitFlag); // Wait for EEPROM address to transmit
    TransmitFlag = 0;
    EUSCI_B0 -> TXBUF = HiAddress; // Send the high byte of the memory address
    while (!TransmitFlag); // Wait for the transmit to complete
    TransmitFlag = 0;
    EUSCI_B0 -> TXBUF = LoAddress; // Send the high byte of the memory address
    while (!TransmitFlag); // Wait for the transmit to complete
    TransmitFlag = 0;
    EUSCI_B0 -> TXBUF = MemByte; // Send the byte to store in EEPROM
    while (!TransmitFlag); // Wait for the transmit to complete
    TransmitFlag = 0;
    EUSCI_B0 -> CTLW0 |= EUSCI_B_CTLW0_TXSTP; // I2C stop condition
}

/*
/ Function that reads a single byte from the EEPROM.
/
/ MemAddress - 2 byte address specifies the address in the EEPROM memory
/ ReceiveByte - 1 byte value that is received from the EEPROM
/
/ Procedure :
/ start
/ transmit address+W (control+0) -> ACK (from EEPROM)
/ transmit data (high address) -> ACK (from EEPROM)
/ transmit data (low address) -> ACK (from EEPROM)
/ start
/ transmit address+R (control+1) -> ACK (from EEPROM)
/ transmit data (data) -> NACK (from MSP432)
/ stop
*/
uint8_t ReadEEPROM(uint16_t MemAddress){
    uint8_t ReceiveByte;
    uint8_t HiAddress;
    uint8_t LoAddress;
    HiAddress = MemAddress >> 8;
    LoAddress = MemAddress & 0xFF;
    EUSCI_B0->CTLW0 |= EUSCI_B_CTLW0_TR; // Set transmit mode (write)
    EUSCI_B0->CTLW0 |= EUSCI_B_CTLW0_TXSTT; // I2C start condition
    while (!TransmitFlag); // Wait for EEPROM address to transmit
    TransmitFlag = 0;
    EUSCI_B0 -> TXBUF = HiAddress; // Send the high byte of the memory address
    while (!TransmitFlag); // Wait for the transmit to complete
    TransmitFlag = 0;
    EUSCI_B0 -> TXBUF = LoAddress; // Send the low byte of the memory address
    while (!TransmitFlag); // Wait for the transmit to complete
    TransmitFlag = 0;
    EUSCI_B0->CTLW0 &= ~EUSCI_B_CTLW0_TR; // Set receive mode (read)
    EUSCI_B0->CTLW0 |= EUSCI_B_CTLW0_TXSTT; // I2C start condition (restart)
    // Wait for start to be transmitted
    while ((EUSCI_B0->CTLW0 & EUSCI_B_CTLW0_TXSTT));
    // set stop bit to trigger after first byte
    EUSCI_B0->CTLW0 |= EUSCI_B_CTLW0_TXSTP;
    while (!TransmitFlag); // Wait to receive a byte
    TransmitFlag = 0;
    ReceiveByte = EUSCI_B0->RXBUF; // Read byte from the buffer
    return ReceiveByte;
}

/*
/ I2C Interrupt Service Routine
*/
void EUSCIB0_IRQHandler(void){
    if (EUSCI_B0->IFG & EUSCI_B_IFG_TXIFG0) // Check if transmit complete
    {
        EUSCI_B0->IFG &= ~ EUSCI_B_IFG_TXIFG0; // Clear interrupt flag
        TransmitFlag = 1; // Set global flag
    }
    if (EUSCI_B0->IFG & EUSCI_B_IFG_RXIFG0) // Check if receive complete
    {
        EUSCI_B0->IFG &= ~ EUSCI_B_IFG_RXIFG0; // Clear interrupt flag
        TransmitFlag = 1; // Set global flag
    }
}
