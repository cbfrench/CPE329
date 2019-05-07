#include "msp.h"
#include "delay.h"
#include "keypad.h"
#include "math.h"
#include "dynamicArray.h"
#include "lcd.h"

#define SHUTDOWN BIT4
#define GAIN BIT5
#define CS_SPI BIT0

#define FREQ_100_Hz 30000
#define FREQ_200_Hz 20000
#define FREQ_300_Hz 10000
#define FREQ_400_Hz 7500
#define FREQ_500_Hz 6000

#define LENGTH 60000

int SAMPLE_RATE = 600;

int sampleRate_500Hz = 118;
int sampleRate_400Hz = 148;
int sampleRate_300Hz = 198;
int sampleRate_200Hz = 295;
int sampleRate_100Hz = 600;

uint16_t voltage = 0;
int8_t factor = 1;
int length;
float duty_cycle = 0;
int sampleRate_Iterator = 0;
Array lookupTable;

enum wave_type {SINE, SQUARE, SAWTOOTH};
enum wave_type wave = SQUARE;

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
    P1->OUT |= CS_SPI;
    EUSCI_B0->CTLW0 &= ~EUSCI_B_CTLW0_SWRST;
}

/**
 *  Transmits Desired Voltage Output Level to SPI DAC
 *  @param val uniform 16-bit integer
 */
void set_voltage(uint16_t val) {
    int loByte, hiByte;
    loByte = val & 0xFF;                                        //!< Reconfigure single 16-bit level into two, 8-bit words
    hiByte = (val >> 8) & 0x0F;
    hiByte |= (SHUTDOWN|GAIN);                                  //!< Ensure in the 'hiBite' word that Shutdown and Gain Flags aren't set

    P1->OUT &= ~CS_SPI;                                         //!< Begin SPI Transmission
    while(!(EUSCI_B0->IFG & EUSCI_B_IFG_TXIFG));
    EUSCI_B0->TXBUF = hiByte;
    while(!(EUSCI_B0->IFG & EUSCI_B_IFG_TXIFG));
    EUSCI_B0->TXBUF = loByte;
    while(!(EUSCI_B0->IFG & EUSCI_B_IFG_RXIFG));
    P1->OUT |= CS_SPI;                                          //<! End SPI Transmission
}

void TA0_0_IRQHandler(void) {
    if(TIMER_A0->CCTL[0] & TIMER_A_CCTLN_CCIFG){
        TIMER_A0->CCTL[0] &= ~TIMER_A_CCTLN_CCIFG;

        switch(wave)
        {
            case SQUARE:
                TIMER_A0->CCR[0] += length;
                break;
            default:
                TIMER_A0->CCR[0] += length;
                break;
        }

        factor *= -1;
    }
}

void TA0_N_IRQHandler(void) {
    if (TIMER_A0->CCTL[1] & TIMER_A_CCTLN_CCIFG) {
        TIMER_A0->CCTL[1] &= ~TIMER_A_CCTLN_CCIFG;
        TIMER_A0->CCR[1] += SAMPLE_RATE *2;

        voltage = lookupTable.array[sampleRate_Iterator];

        sampleRate_Iterator++;
        if(sampleRate_Iterator >= lookupTable.used) sampleRate_Iterator = 0;
    }
}

void init_timers(){
    TIMER_A0->CTL |= TIMER_A_CTL_TASSEL_2 | TIMER_A_CTL_MC_2 | TIMER_A_CTL_ID_1;
    TIMER_A0->CCR[0] = length;
    TIMER_A0->CCR[1] = SAMPLE_RATE*2;
    TIMER_A0->CCTL[0] |= TIMER_A_CCTLN_CCIE;
    TIMER_A0->CCTL[1] |= TIMER_A_CCTLN_CCIE;
    NVIC->ISER[0] = 1 << (TA0_0_IRQn & 31);
    NVIC->ISER[0] = 1 << (TA0_N_IRQn & 31);
}

void init_keypad(){
    //set up pins for keypad rows
    P4 -> SEL0 &= ~ (ROW1 | ROW2 | ROW3 | ROW4);
    P4 -> SEL1 &= ~(ROW1 | ROW2 | ROW3 | ROW4);
    P4 -> DIR &= ~(ROW1 | ROW2 | ROW3 | ROW4);
    P4 -> REN |= ROW1 | ROW2 | ROW3 | ROW4;
    P4 -> OUT &= ~(ROW1 | ROW2 | ROW3 | ROW4);

    //set up pins for keypad columns
    P5 -> SEL0 &= ~ (COL1 | COL2 | COL3);
    P5 -> SEL1 &= ~(COL1 | COL2 | COL3);
    P5 -> DIR |= COL1 | COL2 | COL3;

    //TEST ONLY
    P6 -> SEL0 &= ~BIT4;
    P6 -> SEL1 &= ~BIT4;
    P6 -> DIR |= BIT4;
}
/**
 * Automatically Recalculates each Lookup table
 */

void recalculateLookup()
{
    __disable_irq();                                // Temporarily Disable Interrupts
    int iterator = 0;
    freeArray(&lookupTable);                        // De-Allocate LookupTable if already initialized
    initArray(&lookupTable, 10);                    // Re-Allocate LookupTable

    switch(wave)
    {
        case SINE:
            while(iterator <= length)
            {
              insertArray(&lookupTable, (uint16_t)(2047*sin(((2*3.14159)/length)*iterator)+2047));
              iterator += (length/(50));
            }
            break;
        case SAWTOOTH:
            while(iterator <= length)
            {
                insertArray(&lookupTable, (uint16_t)(4095*((float)iterator/length)));
                iterator += (length/(50));
            }
            break;
        default:   // SQUARE
            while(iterator <= length)
            {
                if(iterator <= length*duty_cycle) insertArray(&lookupTable, 4095);
                else insertArray(&lookupTable, 0);

                iterator += (length/(50));
            }
            break;
    }



    __enable_irq();                                 // Re-Enable Interrupts
}

void change_duty_cycle(int direction){
    //direction is 0 for decrement, 1 for increment
    switch(duty_cycle){
        case 0.1:
            if(direction){
                duty_cycle = 0.2;
            }
            else{
                duty_cycle = 0.1;
            }
            break;
        case 0.2:
            if(direction){
                duty_cycle = 0.3;
            }
            else{
                duty_cycle = 0.1;
            }
            break;
        case 0.3:
            if(direction){
                duty_cycle = 0.4;
            }
            else{
                duty_cycle = 0.2;
            }
            break;
        case 0.4:
            if(direction){
                duty_cycle = 0.5;
            }
            else{
                duty_cycle = 0.3;
            }
            break;
        case 0.5:
            if(direction){
                duty_cycle = 0.6;
            }
            else{
                duty_cycle = 0.4;
            }
            break;
        case 0.6:
            if(direction){
                duty_cycle = 0.7;
            }
            else{
                duty_cycle = 0.5;
            }
            break;
        case 0.7:
            if(direction){
                duty_cycle = 0.8;
            }
            else{
                duty_cycle = 0.6;
            }
            break;
        case 0.8:
            if(direction){
                duty_cycle = 0.9;
            }
            else{
                duty_cycle = 0.7;
            }
            break;
        case 0.9:
            if(direction){
                duty_cycle = 0.9;
            }
            else{
                duty_cycle = 0.8;
            }
            break;
    }
}

void main(void) {
    set_DCO(FREQ_12_MHz);
    length = FREQ_100_Hz;
    duty_cycle = 0.5;
    init_timers();
    init_LCD();
    init_SPI();
    init_keypad();
    set_voltage(0);
    __enable_irq();
    recalculateLookup();
    while(1) {
        int key = check_key_pressed();
        set_voltage(voltage);
        switch(key){
            case('1'):
                length = FREQ_100_Hz;
                SAMPLE_RATE = sampleRate_100Hz;
                recalculateLookup();
                break;
            case('2'):
                length = FREQ_200_Hz;
                SAMPLE_RATE = sampleRate_200Hz;
                recalculateLookup();
                break;
            case('3'):
                length = FREQ_300_Hz;
                SAMPLE_RATE = sampleRate_300Hz;
                recalculateLookup();
                break;
            case('4'):
                length = FREQ_400_Hz;
                SAMPLE_RATE = sampleRate_400Hz;
                recalculateLookup();
                break;
            case('5'):
                length = FREQ_500_Hz;
                SAMPLE_RATE = sampleRate_500Hz;
                recalculateLookup();
                break;
            case('7'):
                wave = SQUARE;
                recalculateLookup();
                break;
            case('8'):
                wave = SINE;
                recalculateLookup();
                break;
            case('9'):
                wave = SAWTOOTH;
                recalculateLookup();
                break;
            case('*'):
                change_duty_cycle(0);
                //delay_ms(200);
                recalculateLookup();
                break;
            case('0'):
                duty_cycle = 0.5;
                recalculateLookup();
                break;
            case('#'):
                change_duty_cycle(1);
                //delay_ms(200);
                recalculateLookup();
                break;
            default:
                break;
        }
    }
}
