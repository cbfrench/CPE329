/*
 * lcd.c
 *
 *  Created on: Apr 11, 2019
 *      Author: conno
 */
#include <string.h>
#include "msp.h"
#include "delay.h"

#define LCD_CONTROL P3
#define LCD_DATA P4
#define RS BIT5
#define RW BIT6
#define E BIT7
#define DISPLAY_CLEAR 0x01
#define NORMAL_ENTRY_MODE 0x06
#define DISPLAY_ON 0x0C
#define CURSOR_ON 0x0A
#define CURSOR_BLINK 0x09

void initLCD();
void functionSet(char input);
void displayOn();
void clearLCD();
void setEntryMode();
void homeLCD();
void writeLCD(char input);
void writeStringLCD(char *input);
void sendByte(char byte);

void initLCD(){
    delay_us(40000);
    LCD_CONTROL->OUT &= ~E;
    LCD_CONTROL->OUT &= ~(RS|RW);
    LCD_DATA->OUT = 0x30;
    LCD_CONTROL->OUT |= E;
    delay_us(0);
    LCD_CONTROL->OUT &= ~E;
    delay_us(40);
    functionSet(0x28);
    functionSet(0x28);
    displayOn();
    clearLCD();
    setEntryMode();
    homeLCD();
}

void functionSet(char input){
    LCD_CONTROL->OUT &= ~E;
    LCD_CONTROL->OUT &= ~(RS|RW);
    sendByte(input);
}

void displayOn(){
    LCD_CONTROL->OUT &= ~(RS|RW);
    LCD_CONTROL->OUT &= ~E;
    sendByte(DISPLAY_ON | CURSOR_ON | CURSOR_BLINK);
}

void clearLCD(){
    LCD_CONTROL->OUT &= ~(RS|RW);
    LCD_CONTROL->OUT &= ~E;
    sendByte(DISPLAY_CLEAR);
    delay_us(1600);
}

void setEntryMode(){
    LCD_CONTROL->OUT &= ~(RS|RW);
    LCD_CONTROL->OUT &= ~E;
    sendByte(NORMAL_ENTRY_MODE);
}

void sendByte(char byte){
    char topBits = byte & 0xF0;
    char bottomBits = byte & 0x0F;
    topBits = topBits >> 4;
    LCD_CONTROL->OUT &= ~E;
    LCD_DATA->OUT &= 0xF0;
    LCD_DATA->OUT |= topBits;
    LCD_CONTROL->OUT |= E;
    delay_us(0);
    LCD_CONTROL->OUT &= ~E;
    delay_us(40);
    LCD_CONTROL->OUT &= ~E;
    LCD_DATA->OUT &= 0xF0;
    LCD_DATA->OUT |= bottomBits;
    LCD_CONTROL->OUT |= E;
    delay_us(0);
    LCD_CONTROL->OUT &= ~E;
    delay_us(40);
}

void homeLCD(){
    LCD_CONTROL->OUT &= ~(RS|RW);
    LCD_CONTROL->OUT &= ~E;
    sendByte(0x00 | BIT7);
}

void changeLine(){
    LCD_CONTROL->OUT &= ~(RS|RW);
    LCD_CONTROL->OUT &= ~E;
    sendByte(0x40 | BIT7);
}

void writeLCD(char input){
    LCD_CONTROL->OUT |= RS;
    LCD_CONTROL->OUT &= ~RW;
    LCD_CONTROL->OUT &= ~E;
    sendByte(input);
    delay_us(40);
}
void writeStringLCD(char *input){
    int i;
    for(i = 0; i < strlen(input); i++){
        writeLCD(input[i]);
    }
}


