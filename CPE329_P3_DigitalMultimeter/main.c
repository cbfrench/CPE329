#include "msp.h"
#include "delay.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

#define SQUARE_PORT P4
#define SQUARE_BIT BIT3

int adc_flag = 0;
volatile uint16_t readValue;
volatile double writeValue;
volatile uint64_t freq_counter;
volatile uint16_t freq_acquire = 0;
volatile uint16_t input_delay = 0;
volatile uint16_t edges_hit = 0;

int flag = 0;
char inValue[5];
int index = 0;

#define NUMBER_OF_SAMPLES 1000
#define UPDATE_INTERFACE 1000
#define PERIOD_CONSTANT 16

float inputMin = 1000;
float inputMax = -1000;
float adjustedMin = 1000;
float adjustedMax = -1000;
float input[NUMBER_OF_SAMPLES];
float input_squared[NUMBER_OF_SAMPLES];
int approximate_square[NUMBER_OF_SAMPLES];
float input_noise_removed[NUMBER_OF_SAMPLES];
float threshold;
float raw_middle;
float middle;
int sample = 0;
double period;
double frequency;
double vpp = 0;
double vrms = 0;
int period_start;
int period_end;

float SUBTRACTION_CONSTANT = 0.35;

int count = 0;
int update_interface = 0;

enum wave_type {SINE, SQUARE, TRIANGLE};
enum wave_type wave;

enum mode_type {AC, DC};
enum mode_type mode = DC;

enum terminalColors {BLK = 30, RED = 31, GRN = 32, YEL = 33, BLU = 34, MAG = 35, CYN = 36, WHT = 37};

double max(double a, double b){
    if(a > b){
        return a;
    }
    return b;
}

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
    threshold = (inputMax - inputMin) / 100;   //sets threshold to 1% of difference
    raw_middle = (inputMax + inputMin) / 2;       //get rough average, will refine later
    //vpp = inputMax - inputMin;
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
    middle = (adjustedMax + adjustedMin) / 2 + adjustedMin;   //get finer average
    vpp = adjustedMax - adjustedMin;
}

/**
 * Generates an approximate wave data from input
 */
void approximate_wave(){
    int i;
    for(i = 0; i < NUMBER_OF_SAMPLES; i++){
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
    freq_acquire = 1;
    P4->IE |= BIT3;
    _delay_cycles(24000000);
    P4->IE &= ~BIT3;
    freq_acquire = 0;
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
            | ADC14_CTL0_SSEL__HSMCLK
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
    P1->OUT ^= BIT0;
}

void ADC_16_cycles(){
    writeValue = 0.0002 * readValue + 0.0001;       //set output value for 16 cycles
}

void print_freq(float input){
    int f = input;
    double digit;
    char digits[4];
    int count = 0;
    int i;
    while(f > 0 && count < 4){
        digits[count] = f % 10 + '0';
        f /= 10;
        count++;
    }
    for(i = 3; i >= 0; i--){
        print_char(digits[i]);
    }
}

void print_float(float input){
    float f = input;
    int count = 0;
    int round;
    if(f < 0){
        print_char("-");
        f = -f;
    }
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

float get_average(){
    int i;
    float sum = 0;
    for(i = 0; i < sample+1; i++){
        sum += input[i];
    }

    return sum / sample;
}

float approximate_vrms(){
    float return_value = 0;
    int i;
    int square = 1;
    for(i = 0; i < NUMBER_OF_SAMPLES; i++){
        return_value += (input[i] - inputMin);
        if(raw_middle + threshold * 20 > input[i] && raw_middle - threshold * 20 < input[i]){
            square = 0;
        }
    }
    if(square){
        SUBTRACTION_CONSTANT = 0.275;
    }
    else if(inputMax - inputMin > 1.8){
        //higher voltages
        SUBTRACTION_CONSTANT = 0.2 + (inputMax - inputMin - 3) * -0.1;
    }
    else{
        SUBTRACTION_CONSTANT = 0.35;
    }
    return (vrms + sqrt(return_value / NUMBER_OF_SAMPLES) - SUBTRACTION_CONSTANT) / 2;
}

/**
 *  Creates the Terminal-Based DMM interface
 */
void generate_interface(){
    static int warningColor = 0;
    clear_terminal();

    /*if(mode == FREQ){
        // Write Frequency To Terminal
        print_string("    Frequency: ");
        print_freq(frequency);
        print_line(" Hz \n\r");
        print_newline();
    }*/
    if (mode == DC){
        // Change Screen Color if Voltage is Too High
        if(warningColor != 1 && writeValue >= 3.260) {
            warningColor = 1;                                       // Set warningColor Flag
            color_terminal(WHT, RED);                               // Turn Background Red
        }
        else if (warningColor == 1 && writeValue < 3.260) {
            warningColor = 0;                                       // Unset warningColor Flag
            color_terminal(WHT, BLK);                               // Turn Background Black
        }

        // Write Voltage and RMS Bar
        print_string("  Voltage RMS: ");                            // Label Voltage
        print_string(rmsBarGenerator(writeValue));                     // Write RMS Bar
        print_string("      ");
        print_float(writeValue);                                       // Write Voltage Number
        print_line(" V \n\r");
        print_line("              0      1     2     3   3.3");     // Write RMS Bar Graduations
    }
    else{
        // Change Screen Color if Voltage is Too High
        if(warningColor != 1 && vpp >= 3.260) {
            warningColor = 1;                                       // Set warningColor Flag
            color_terminal(WHT, RED);                               // Turn Background Red
        }
        else if (warningColor == 1 && vrms < 3.260) {
            warningColor = 0;                                       // Unset warningColor Flag
            color_terminal(WHT, BLK);                               // Turn Background Black
        }

        // Write Voltage and RMS Bar
        print_string("  Voltage RMS: ");                            // Label Voltage
        print_string(rmsBarGenerator(vrms));                     // Write RMS Bar
        print_string("      ");
        print_float(vrms);                                       // Write Voltage Number
        print_line(" V \n\r");
        print_line("              0      1     2     3   3.3");     // Write RMS Bar Graduations
        print_string("    Frequency: ");
        print_freq(frequency);
        print_line(" Hz \n\r");
        print_newline();
    }

    print_newline();
    print_newline();
    print_string("  Measurement Mode: ");                       // Label Measurement Mode
    if(mode == AC) print_line("DC   >AC<");      // acdc_modeFlag = 0, measure AC
    else if(mode == DC) print_line(">DC<   AC"); // acdc_modeFlag = 1 , measure DC
    else print_line("DC    AC  >Freq<");
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
            vrms = approximate_vrms();
            inputMin = 1000;
            inputMax = -1000;
            adjustedMin = 1000;
            adjustedMax = -1000;
            sample = 0;
            //printf("INPUT MAX: %f, INPUT MIN: %f\n", inputMax, inputMin);
            return;
        }
        else{
            ADC_16_cycles();    //16 cycles to sample once
            input[sample] = writeValue;
            input_squared[sample] = writeValue * writeValue;
            approximate_square[sample] = (SQUARE_PORT->IN & SQUARE_BIT) >> 3;
            sample++;
        }
    }
    /*
    else if(mode == FREQ){
        if(sample >= NUMBER_OF_SAMPLES){
            //sample 100 times at 16 cycles each: 1600 cycles at 3MHz: less than 1ms per update
            get_input_min_max();
            approximate_wave();
            get_adjusted_min_max();
            get_frequency();
            vrms = approximate_vrms();
            inputMin = 1000;
            inputMax = -1000;
            adjustedMin = 1000;
            adjustedMax = -1000;
            sample = 0;
            //printf("Min: %f, Max: %f\n", inputMin, inputMax);
            return;
        }
        else{
            P1->OUT ^= BIT0;
            ADC_16_cycles();    //16 cycles to sample once
            input[sample] = writeValue;
            input_squared[sample] = writeValue * writeValue;
            approximate_square[sample] = (SQUARE_PORT->IN & SQUARE_BIT) >> 3;  //using 4.3 as square wave input
            sample++;
        }
    }*/
    else{
        sample = 0;
        //DC mode
        ADC_16_cycles();
    }
}

void TA0_0_IRQHandler(void){
    if(TIMER_A0->CCTL[0] & TIMER_A_CCTLN_CCIFG){
        TIMER_A0->CCTL[0] &= ~TIMER_A_CCTLN_CCIFG;
        if(P4->IN & BIT3){
            if(freq_acquire){
                period = 2.0 / edges_hit / 0.91;
                frequency = 1 / period;
                freq_acquire = 0;
                edges_hit = 0;
            }
        }
        TIMER_A0->CCR[0] += 10;
    }
}

void PORT4_IRQHandler(void){
    edges_hit++;
    P4->IFG &= ~BIT3;
}

void main(void)
{
    WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;     // stop watchdog timer
    set_DCO(FREQ_24_MHz);

    init_UART();
    init_ADC();

    //set up timer for determining frequency
    TIMER_A0->CTL |= TIMER_A_CTL_SSEL__SMCLK | TIMER_A_CTL_MC_2 | TIMER_A_CTL_ID_3 | TIMER_A_CTL_IE; //timer using SMCLK, up mode, no divider
    TIMER_A0->CCR[0] = 500;
    TIMER_A0->CCTL[0] |= TIMER_A_CCTLN_CCIE;        // Enable Interrupts

    //set up pin for incoming square wave
    P4->SEL0 &= ~BIT3;
    P4->SEL1 &= ~BIT3;
    P4->DIR &= ~BIT3;
    P4->IES &= ~BIT3;
    P4->IFG &= ~BIT3;
    P4->IE &= ~BIT3;

    NVIC->ISER[0] = (1 << (ADC14_IRQn & 31));
    NVIC->ISER[1] = (1 << (EUSCIA0_IRQn & 31));
    NVIC->ISER[0] = (1 << (TA0_0_IRQn & 31));
    NVIC->ISER[1] = (1 << (PORT4_IRQn & 31));

    __enable_irq();

    P6->SEL0 |= BIT1;   // Setup P6.1 for ADC Input
    P6->SEL1 |= BIT1;

    P2->SEL0 &= ~BIT7;  // Setup P2.7 for GPIO Input (AC Switch)
    P2->SEL1 &= ~BIT7;
    P2->DIR &= ~BIT7;
    P2->REN |= BIT7;
    P2->OUT |= BIT7;

    P2->SEL0 &= ~BIT5;  // Setup P2.5 for GPIO Input (FREQ Switch)
    P2->SEL1 &= ~BIT5;
    P2->DIR &= ~BIT5;
    P2->REN |= BIT5;
    P2->OUT |= BIT5;

    //set up for testing
    P1->SEL0 &= ~BIT0;
    P1->SEL1 &= ~BIT0;
    P1->DIR |= BIT0;






    ADC14->CTL0 |= ADC14_CTL0_SC;                   //start conversion
    color_terminal(WHT, BLK);
    while(1){
        if(!(P2->IN & BIT7)) mode = AC;             // Switch operating mode based on GPIO input
        //else if(!(P2->IN & BIT5)) mode = FREQ;
        else mode = DC;

        if(adc_flag){
            adc_flag = 0;                           //reset flag
            get_voltage();
            //vrms = approximate_vrms();
            //P1->OUT ^= BIT0;
            //ADC_16_cycles();       //use 16 cycles
            update_interface++;
            if(update_interface >= UPDATE_INTERFACE){
                generate_interface();
                update_interface = 0;
            }

            ADC14->CTL0 |= ADC14_CTL0_SC;           //start conversion
        }
        delay_us(100);                           //delay
    }
}
