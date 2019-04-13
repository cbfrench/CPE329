/*
 * lcd.h
 *
 *  Created on: Apr 11, 2019
 *      Author: conno
 */

#ifndef LCD_H_
#define LCD_H_

#define D04 BIT4
#define D05 BIT5
#define D06 BIT6
#define D07 BIT7

#define RS BIT5
#define RW BIT6
#define E  BIT7

#define LCD_Control P3
#define LCD_Data P4

void initLCD();
void Write_char_LCD(char character);
void writeString(char* string);
void changeLine();
void Clear_LCD();
void Home_LCD();

#endif /* LCD_H_ */
