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
    if(key == 11) key = 0;
    key += '0';
    if(key == 0x3A) key = '*';
    if(key == 0x3C) key = '#';
    return key;
}
