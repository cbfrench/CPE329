/*
 * keypad.h
 *
 *  Created on: Apr 13, 2019
 *      Author: conno
 */

#ifndef KEYPAD_H_
#define KEYPAD_H_

#define ROWS P2
#define COLS P5

#define ROW1 BIT4
#define ROW2 BIT5
#define ROW3 BIT6
#define ROW4 BIT7
#define COL1 BIT0
#define COL2 BIT1
#define COL3 BIT2

int check_key_pressed();

#endif /* KEYPAD_H_ */
