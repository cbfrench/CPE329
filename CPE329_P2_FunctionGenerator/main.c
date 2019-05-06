#include "msp.h"
#include "delay.h"
#include "keypad.h"

#define SHUTDOWN BIT4
#define GAIN BIT5
#define CS BIT0

#define FREQ_100_Hz 30000
#define FREQ_200_Hz 15000
#define FREQ_300_Hz 10000
#define FREQ_400_Hz 7500
#define FREQ_500_Hz 6000

#define LENGTH 60000
#define SAMPLE_RATE 25

volatile uint16_t voltage = 0;
volatile int8_t factor = 1;
int length = LENGTH;
float duty_cycle = 0.5;

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
    P1->OUT |= CS;
    EUSCI_B0->CTLW0 &= ~EUSCI_B_CTLW0_SWRST;
}

void set_voltage(uint16_t val) {
    int loByte, hiByte;
    loByte = val & 0xFF;
    hiByte = (val >> 8) & 0x0F;
    hiByte |= (SHUTDOWN|GAIN);
    P1->OUT &= ~CS;
    while(!(EUSCI_B0->IFG & EUSCI_B_IFG_TXIFG));
    EUSCI_B0->TXBUF = hiByte;
    while(!(EUSCI_B0->IFG & EUSCI_B_IFG_TXIFG));
    EUSCI_B0->TXBUF = loByte;
    while(!(EUSCI_B0->IFG & EUSCI_B_IFG_RXIFG));
    P1->OUT |= CS;
}

void TA0_0_IRQHandler(void) {
    TIMER_A0->CCTL[0] &= ~TIMER_A_CCTLN_CCIFG;
    if(wave != SQUARE){
        if(wave == SINE){
            TIMER_A0->CCR[0] += approximate_sine(length);
        }
        else{
            TIMER_A0->CCR[0] += length;
        }
    }
    else{
        if(factor == 1){
            TIMER_A0->CCR[0] += length;
        }
        else{
            TIMER_A0->CCR[0] += length * (1 - duty_cycle) * 2;
        }
    }
    if(wave != SAWTOOTH){
        factor *= -1;
    }
    else{
        voltage = 0;
    }
}

void TA0_N_IRQHandler(void) {
    if (TIMER_A0->CCTL[1] & TIMER_A_CCTLN_CCIFG) {
        TIMER_A0->CCTL[1] &= ~TIMER_A_CCTLN_CCIFG;
        TIMER_A0->CCR[1] += SAMPLE_RATE * 2;

        if(wave == SQUARE){
            set_square();
        }
        else if(wave == SINE){
            set_sine();
        }
        else{
            set_sawtooth();
        }
    }
}

int approximate_sine(int x){
    float PI = 3.1415926;
    float PI_SQUARED = 9.8696044;
    float numerator = 16 * x * (PI - x);
    float denominator = 5 * PI_SQUARED - 4 * x * (PI - x);
    return (int)(numerator/denominator);
}

void set_square(){
    voltage = 3200 * (factor + 1) * 2;
}

void set_sine(){
    voltage += factor * 2;
}
void set_sawtooth(){
    voltage += factor * 2;
}

void init_timers(){
    TIMER_A0->CTL |= TIMER_A_CTL_TASSEL_2 | TIMER_A_CTL_MC_2 | TIMER_A_CTL_ID_1;
    TIMER_A0->CCR[0] = length;
    TIMER_A0->CCR[1] = SAMPLE_RATE;
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
}

void main(void) {
    set_DCO(FREQ_12_MHz);
    length = FREQ_100_Hz;
    init_timers();
    init_SPI();
    init_keypad();
    set_voltage(0);
    __enable_irq();
    while(1) {
        int key = check_key_pressed();
        switch(key){
            case('1'):
                length = FREQ_100_Hz;
                break;
            case('2'):
                length = FREQ_200_Hz;
                break;
            case('3'):
                length = FREQ_300_Hz;
                break;
            case('4'):
                length = FREQ_400_Hz;
                break;
            case('5'):
                length = FREQ_500_Hz;
                break;
            case('7'):
                wave = SQUARE;
                break;
            case('8'):
                wave = SINE;
                voltage = 0;
                factor = 1;
                break;
            case('9'):
                wave = SAWTOOTH;
                factor = 1;
                break;
            case('*'):
                duty_cycle -= 0.1;
                if(duty_cycle <= 0){
                    duty_cycle = 0.1;
                }
                break;
            case('0'):
                duty_cycle = 0.5;
                break;
            case('#'):
                duty_cycle += 0.1;
                if(duty_cycle >= 1){
                    duty_cycle = 0.9;
                }
                break;
            default:
                break;
        }
        set_voltage(voltage);
    }
}
