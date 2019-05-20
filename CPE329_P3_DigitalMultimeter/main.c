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

#define NUMBER_OF_SAMPLES 100

float inputMin = 1000;
float inputMax = -1000;
float adjustedMin = 1000;
float adjustedMax = -1000;
float input[NUMBER_OF_SAMPLES];
int approximate_square[NUMBER_OF_SAMPLES];
float input_noise_removed[NUMBER_OF_SAMPLES];
float threshold;
float middle;
int sample = 0;
double frequency;

enum wave_type {SINE, SQUARE, TRIANGLE};
enum wave_type wave;

enum mode_type {AC, DC};
enum mode_type mode = AC;

/**
 * Gets min and max input sampled and stores in globals
 */
void get_input_min_max(){
    int i;
    for(i = 0; i < NUMBER_OF_SAMPLES; i++){
        if(input[i] < inputMin){
            inputMin = input[i];
        }
        if(input[i] > inputMax){
            inputMax = input[i];
        }
    }
    threshold = (inputMax - inputMin) / 10;   //sets threshold to 10% of difference
    middle = (inputMax + inputMin) / 2;       //get rough average, will refine later
}

/**
 * Gets adjusted min and max input
 */
void get_adjusted_min_max(){
    int i;
    for(i = 0; i < NUMBER_OF_SAMPLES; i++){
        if(input_noise_removed[i] < adjustedMin){
            adjustedMin = input_noise_removed[i];
        }
        if(input_noise_removed[i] > adjustedMax){
            adjustedMax = input_noise_removed[i];
        }
    }
    middle = (adjustedMax + adjustedMin) / 2;   //get finer average
}

/**
 * Generates an approximate wave data from input
 */
void approximate_wave(){
    int i;
    for(i = 0; i < NUMBER_OF_SAMPLES; i++){
        if(input[i] > threshold + middle){
            approximate_square[i] = 1;      //if above the required threshold, square wave is positive
        }
        else if(input[i] < -threshold + middle){
            approximate_square[i] = -1;     //if below the required threshold, square wave is negative
        }
        else{
            approximate_square[i] = 0;      //data cannot be trusted
        }
        if(i != 0){
            if(input[i-1] > input[i]){
                if(input[i] + threshold > input[i-1])
                {
                    input_noise_removed[i] = input[i-1];    //if previous is greater than current and within 10%, set it
                }
                else{
                    input_noise_removed[i] = middle;        //data cannot be trusted
                }
            }
            else{
                if(input[i] - threshold < input[i-1]){
                    input_noise_removed[i] = input[i-1];    //if previous is less than current and within 10%, set it
                }
                else{
                    input_noise_removed[i] = middle;        //data cannot be trusted
                }
            }
        }
    }
}

void get_frequency(){
    /**
     * find first switch, then start counting until next switch
     * use information about run speed and sample rate to determine frequency
     */
    int i;
    int found_flip = 0;
    int num_samples = 0;
    double period;
    for(i = 1; i < NUMBER_OF_SAMPLES; i++){
        if(found_flip){
            if(approximate_square[i-1] == -approximate_square[i]){
                found_flip = 1;
            }
        }
        else{
            if(approximate_square[i-1] == -approximate_square[i]){
                found_flip = 0;
                period = (double)(num_samples * 16) / 3000000; //(number of samples * number of cycles per sample) / number of cycles a second
                frequency = 1 / period;
                return;
            }
            else{
                num_samples++;
            }
        }
    }
    //if it makes it here, sampling rate is too low/wave is too fast
    frequency = 0;
}

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

void ADC_16_cycles(){
    writeValue = 0.0002 * readValue + 0.0001;       //set output value for 16 cycles
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

/*
 * Main function for running the DMM
 */
void get_voltage(){
    if(mode == AC){
        if(sample >= NUMBER_OF_SAMPLES){
            //sample 100 times at 16 cycles each: 1600 cycles at 3MHz: less than 1ms per update
            get_input_min_max();
            approximate_wave();
            get_adjusted_min_max();
            get_frequency();
            sample = 0;
            //print out Vpp and Wrms somehow
            return;
        }
        else{
            ADC_16_cycles();    //16 cycles to sample once
            input[sample] = writeValue;
            sample++;
        }
    }
    else{
        if(sample >= NUMBER_OF_SAMPLES){
            get_input_min_max();
            approximate_wave();
            get_adjusted_min_max();
            //print average voltage
            sample = 0;
            return;
        }
        else{
            ADC_16_cycles();    //16 cycles to sample once
            input[sample] = writeValue;
            sample++;
        }
    }
}

void main(void)
{
    WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;     // stop watchdog timer
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
            get_voltage();
            print_float(writeValue);                //print converted value to UART
            ADC14->CTL0 |= ADC14_CTL0_SC;           //start conversion
        }
        delay_us(100000);                           //delay
    }
}
