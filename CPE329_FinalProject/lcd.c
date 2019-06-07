#include <string.h>
#include <stdint.h>
#include "msp.h"
#include "delay.h"
#include "lcd.h"

void _write_data(uint8_t data){
    LCD_CONTROL -> OUT &= ~(RS | RW);
    LCD_CONTROL -> OUT &= ~E;
    LCD_DATA -> OUT = data & 0xF0;
    _pulse();
    LCD_DATA -> OUT = data << 4;
    if(data == DISPLAY_CLEAR){  //special case to prevent timing issues on lower frequencies
        LCD_DATA -> OUT = data;
    }
    _pulse();
    delay_us(40);
}

void _write_data_long(uint8_t data){
    LCD_CONTROL -> OUT &= ~(RS | RW);
    LCD_CONTROL -> OUT &= ~E;
    LCD_DATA -> OUT = data & 0xF0;
    _pulse_long();
    LCD_DATA -> OUT = data << 4;
    if(data == DISPLAY_CLEAR){  //special case to prevent timing issues on lower frequencies
        LCD_DATA -> OUT = data;
    }
    _pulse_long();
    delay_us(1500);
}

void _initial_function_set(){
    LCD_CONTROL -> OUT &= ~E;
    LCD_CONTROL -> OUT &= ~(RS | RW);
    LCD_DATA -> OUT = FIRST_FUNCTION_SET;
    _pulse();
    delay_us(40);
}

void init_LCD()
{
    // Initalize LCD GPIO Pins
    P4 -> SEL0 &= ~ (D04 | D05 | D06 | D07);        // Select Data Bits for use as GPIO
    P4 -> SEL1 &= ~(D04 | D05 | D06 | D07);
    P4 -> DIR |= D04 | D05 | D06 | D07;             // Set-up as Outputs

    P3 -> SEL0 &= ~(RS | RW | E);                   //Select Control Bits for use as GPIO
    P3 -> SEL1 &= ~(RS | RW | E);
    P3 -> DIR |= RS | RW | E;                       // Set-Up as Outputs

    // Begin Communication Protocol Init
    delay_us(40000);

    _initial_function_set();

    _write_data(FUNCTION_SET);
    _write_data(FUNCTION_SET);
    _write_data(DISPLAY_ON);
    _write_data(DISPLAY_CLEAR);
    _write_data(ENTRY_MODE_SET);
}

void write_char_LCD(char character)
{
    LCD_CONTROL -> OUT |= RS;
    LCD_CONTROL -> OUT &= ~(RW | E);
    LCD_DATA -> OUT = character & 0xF0;
    _pulse_long();
    LCD_DATA -> OUT = character << 4;
    _pulse_long();
    delay_us(1500);
}

void write_string(char* string)
{
    int i;

    for(i = 0; i < strlen(string); i++)
    {
        if(string[i] == '\n' || i > 16) change_line();

        write_char_LCD(string[i]);
    }
}

void change_line()
{
    static int lineChanged = 0;

    if(!lineChanged)
    {
        _write_data_long(SECOND_ROW);
    }
    else
    {
        _write_data_long(FIRST_ROW);
    }

    lineChanged = ~lineChanged;
}

void change_to_line(int lineNumber)
{
    if(lineNumber == 1) _write_data_long(FIRST_ROW);
    else if (lineNumber = 2) _write_data_long(SECOND_ROW);
}

void clear_LCD()
{
    _write_data_long(DISPLAY_CLEAR);
}

void home_LCD()
{
    _write_data_long(FIRST_ROW);
}

void _pulse(){
    LCD_CONTROL -> OUT |= E;
    delay_us(0);
    LCD_CONTROL -> OUT &= ~E;
}

void _pulse_long(){
    LCD_CONTROL -> OUT |= E;
    delay_us(1500);
    LCD_CONTROL -> OUT &= ~E;
}









