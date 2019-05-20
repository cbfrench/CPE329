#include "msp.h"
#include "delay.h"
#include "dac.h"
#include <stdio.h>
#include <string.h>

int adc_flag = 0;
int acdc_modeFlag = 0;
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

enum terminalColors {BLK = 30, RED = 31, GRN = 32, YEL = 33, BLU = 34, MAG = 35, CYN = 36, WHT = 37};

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
    for(i = 0; i < NUMBER_OR_SAMPLES; i++){
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
        if(!found_flip){
            if(approximate_square[i-1] != approximate_square[i]){
                found_flip = 1;
            }
        }
        else{
            if(approximate_square[i-1] != approximate_square[i]){
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
}

/**
 * Writes an string of characters, then terminates the line with \CR+\LN
 */
void print_line(char* string){
    print_string(string);

    print_char(((char)0x0A));   // New Line
    print_char(((char)0x0D));   // Carriage Return
}

/**
 * Skips a Line in the Terminal
 */
void print_newline(){
    print_char(((char)0x0A));   // New Line
    print_char(((char)0x0D));   // Carriage Return
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

/**
 * Initialize the Analog-To-Digital Converter Module
 */
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
    print_string(" V \n\r");
}

/**
 * Sends appropriate VT100 Terminal Command to clear Terminal and reset the cursor location
 */
void clear_terminal(){
    print_string("\033[2J");    // ESC [ 2 J
                                // 2 = Clear Entire Display; J = Command Specifier
    print_string("\033[0;0H");  // ESC [ 0 ; 0 H
                                //       X   Y  Coordinates; H = Command Specifier
}

/**
 * Change the color of the Terminal using VT100 Terminal color commands
 */
void color_terminal(enum terminalColors foreColor, enum terminalColors backColor){
    char foreColorTmp[2];
    sprintf(foreColorTmp, "%d", foreColor);     // Convert number to character array
    char backColorTmp[2];
    sprintf(backColorTmp, "%d", backColor+10);  // Convert number to character array

    print_string("\033[");      // ESC [
    print_string(foreColorTmp); // Write Foreground Color
    print_char(';');            // Delimiter
    print_string(backColorTmp); // Write Background Color
    print_char('m');            // Command Specifier (color)
}

/**
 * Depending on input voltage (0 - 3.5), return up to 20 '#' characters on a sliding scale
 */
char* rmsBarGenerator(double voltage){
    if(voltage <= 0)                             return "[                    ]";
    else if(voltage > 0 && voltage <= 0.175)     return "[#                   ]";
    else if(voltage > 0.175 && voltage <= 0.350) return "[##                  ]";
    else if(voltage > 0.350 && voltage <= 0.525) return "[###                 ]";
    else if(voltage > 0.525 && voltage <= 0.700) return "[####                ]";
    else if(voltage > 0.700 && voltage <= 0.875) return "[#####               ]";
    else if(voltage > 0.875 && voltage <= 1.050) return "[######              ]";
    else if(voltage > 1.050 && voltage <= 1.225) return "[#######             ]";
    else if(voltage > 1.225 && voltage <= 1.400) return "[########            ]";
    else if(voltage > 1.400 && voltage <= 1.575) return "[#########           ]";
    else if(voltage > 1.575 && voltage <= 1.750) return "[##########          ]";
    else if(voltage > 1.750 && voltage <= 1.925) return "[###########         ]";
    else if(voltage > 1.925 && voltage <= 2.100) return "[############        ]";
    else if(voltage > 2.100 && voltage <= 2.275) return "[#############       ]";
    else if(voltage > 2.275 && voltage <= 2.450) return "[##############      ]";
    else if(voltage > 2.450 && voltage <= 2.625) return "[###############     ]";
    else if(voltage > 2.625 && voltage <= 2.800) return "[################    ]";
    else if(voltage > 2.800 && voltage <= 2.975) return "[#################   ]";
    else if(voltage > 2.975 && voltage <= 3.150) return "[##################  ]";
    else if(voltage > 3.150 && voltage <= 3.260) return "[################### ]";
    else if (voltage > 3.260)                    return "[!!!!! TOO HIGH !!!!!]";

    return "[!!!!!!!!!!!!!!!!!!!!]";    // Means an Error Occurred
}

/**
 *  Creates the Terminal-Based DMM interface
 */
void generate_interface(double voltage){
    static int warningColor = 0;

    // Write Frequency To Terminal
    clear_terminal();
    print_line("    Frequency: ---- Hz");
    print_newline();

    // Change Screen Color if Voltage is Too High
    if(warningColor != 1 && voltage >= 3.260) {
        warningColor = 1;                                       // Set warningColor Flag
        color_terminal(WHT, RED);                               // Turn Background Red
    }
    else if (warningColor == 1 && voltage < 3.260) {
        warningColor = 0;                                       // Unset warningColor Flag
        color_terminal(WHT, BLK);                               // Turn Background Black
    }

    // Write Voltage and RMS Bar
    print_string("  Voltage RMS: ");                            // Label Voltage
    print_string(rmsBarGenerator(voltage));                     // Write RMS Bar
    print_string("      ");
    print_float(voltage);                                       // Write Voltage Number
    print_line("              0      1     2     3   3.3");     // Write RMS Bar Graduations
    print_newline();
    print_newline();
    print_string("  Measurement Mode: ");                       // Label Measurement Mode
    if(acdc_modeFlag == 0) print_line("DC   >AC<   Freq");      // acdc_modeFlag = 0, measure AC
    else if(acdc_modeFlag == 1) print_line(">DC<   AC   Freq"); // acdc_modeFlag = 1 , measure DC
    else print_line("DC    AC  >Freq<")
    print_line("  (Hint: GND P2.7 for AC Mode)");

    print_newline();
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
        //DC mode
        ADC_16_cycles();
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

    P6->SEL0 |= BIT1;   // Setup P6.1 for ADC Input
    P6->SEL1 |= BIT1;

    P2->SEL0 &= ~BIT7;  // Setup P2.7 for GPIO Input (AC/DC Switch)
    P2->SEL1 &= ~BIT7;
    P2->DIR &= ~BIT7;
    P2->REN |= BIT7;
    P2->OUT |= BIT7;

    ADC14->CTL0 |= ADC14_CTL0_SC;                   //start conversion
    color_terminal(WHT, BLK);
    while(1){

        acdc_modeFlag = !(P2->IN & BIT7);           // Set AC/DC Mode Flag to inverse value of P2.7

        if(adc_flag){
            adc_flag = 0;                           //reset flag
            get_voltage();
            //ADC_16_cycles();                        //use 16 cycles

            ADC14->CTL0 |= ADC14_CTL0_SC;           //start conversion
        }
        generate_interface(writeValue);
        delay_us(100000);                           //delay
    }
}
