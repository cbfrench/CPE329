/*
 * lcd.h
 *
 *  Created on: Apr 11, 2019
 *      Author: conno
 */

#ifndef LCD_H_
#define LCD_H_

void initLCD();

void functionSet(char input);

void displayOn();

void clearLCD();

void setEntryMode();

void homeLCD();

void writeLCD(char input);

void writeStringLCD(char *input);

void changeLine();



#endif /* LCD_H_ */
