#include <string.h>
#include <stdint.h>
#include "msp.h"
#include "delay.h"
#include "keypad.h"


int check_key_pressed(){
    uint8_t rows;
    int col, key;
    COLS->OUT |= COL1 | COL2 | COL3;
    _delay_cycles(25);
    rows = (ROWS->IN & (ROW1|ROW2|ROW3|ROW4));
    if(rows == 0) return -1;    //if no key pressed, return -1
    for(col = 0; col < 3; col++){
        COLS->OUT &= ~(COL1|COL2|COL3);
        COLS->OUT |= (BIT0 << col);
        _delay_cycles(25);
        rows = (ROWS->IN & (ROW1|ROW2|ROW3|ROW4));
        if(rows != 0) break;    //if key pressed in the current row, break
    }
    COLS->OUT &= ~(COL1|COL2|COL3);
    if(col == 0) col = 1;
    else if(col == 1) col = 0;
    if(col == 3) return -1; //if no keypress found, return -1
    if(rows == 4) rows = 3;
    if(rows == 8) rows = 4;
    key = rows * 3 + col - 2;
    //key = (rows * col+1) + ((rows)*(3-col+1));
    if(key == 47) key = 0;
    //key += '0';
    //if(key == 46) key = '*';
    //if(key == 48) key = '#';
    return key;
}

void init_keypad(){
    P4 -> SEL0 &= ~ (ROW1 | ROW2 | ROW3 | ROW4);    //<! Setup Input Pins for Rows
    P4 -> SEL1 &= ~(ROW1 | ROW2 | ROW3 | ROW4);
    P4 -> DIR &= ~(ROW1 | ROW2 | ROW3 | ROW4);
    P4 -> REN |= ROW1 | ROW2 | ROW3 | ROW4;
    P4 -> OUT &= ~(ROW1 | ROW2 | ROW3 | ROW4);

    P5 -> SEL0 &= ~ (COL1 | COL2 | COL3);           //<! Setup Output Pins for Columns
    P5 -> SEL1 &= ~(COL1 | COL2 | COL3);
    P5 -> DIR |= COL1 | COL2 | COL3;
}
