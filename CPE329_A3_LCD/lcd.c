/*
 * lcd.c
 *
 *  Created on: Apr 11, 2019
 *      Author: conno
 */
#include <string.h>
#include <stdint.h>
#include "msp.h"
#include "delay.h"

#define LCD_CONTROL P3
#define LCD_DATA P4
#define RS BIT5
#define RW BIT6
#define E BIT7
#define INITIAL_COMMAND 0x33
#define FUNCTION_SET 0x28
#define DISPLAY_ON 0x0F
#define DISPLAY_CLEAR 0x01
#define ENTRY_MODE 0x06
#define SET_LINE 0x80
#define RETURN_HOME 0x02
#define LINE_1 0x00
#define LINE_2 0x40

void write_data(uint8_t data, int delay_time);
void write_char(uint8_t c);
void output(uint8_t data);
void writeStringLCD(char *s);
void initialFunctionSet();
void functionSet();
void displayOn();
void clearLCD();
void entryMode();
void initLCD();
void changeLine(uint8_t lineNumber);
void toggleE();

void write_data(uint8_t data, int delay_time){
    LCD_CONTROL->OUT &= ~(RS|RW);
    LCD_CONTROL->OUT &= ~E;
    output(data);
    delay_us(delay_time);
}

void write_char(uint8_t c){
    LCD_CONTROL->OUT |= RS;
    LCD_CONTROL->OUT &= ~RW;
    LCD_CONTROL->OUT &= E;
    output(c);
    delay_us(40);
}

void output(uint8_t data){
    LCD_DATA->OUT = data >> 4;
    toggleE();
    LCD_DATA->OUT = data;
    toggleE();
}

void writeStringLCD(char *s){
    int i;
    for(i = 0; i < strlen(s); i++){
        write_char(s[i]);
    }
}

void initialFunctionSet(){
    delay_us(40000);
    LCD_CONTROL->OUT &= ~(RS|RW);
    LCD_CONTROL->OUT &= ~E;
    LCD_DATA->OUT = INITIAL_COMMAND;
    toggleE();
    delay_us(40);
}

void functionSet(){
    write_data(FUNCTION_SET, 40);
}

void displayOn(){
    write_data(DISPLAY_ON, 40);
}

void clearLCD(){
    write_data(DISPLAY_CLEAR, 1600);
}

void entryMode(){
    write_data(ENTRY_MODE, 40);
}

void initLCD(){
    initialFunctionSet();
    functionSet();
    functionSet();
    displayOn();
    clearLCD();
    entryMode();
}

void changeLine(uint8_t lineNumber){
    uint8_t l;
    if(lineNumber == 1){
        l = LINE_1;
    }
    else{
        l = LINE_2;
    }
    write_data(SET_LINE | l, 40);
}

void homeLCD(){
    write_data(RETURN_HOME, 1600);
}

void toggleE(){
    LCD_CONTROL->OUT |= E;
    delay_us(0);
    LCD_CONTROL->OUT &= ~E;
}
