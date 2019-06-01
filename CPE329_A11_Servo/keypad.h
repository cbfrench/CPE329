#ifndef KEYPAD_H_
#define KEYPAD_H_

#define ROWS P4
#define COLS P5

#define ROW1 BIT0
#define ROW2 BIT1
#define ROW3 BIT2
#define ROW4 BIT4
#define COL1 BIT1
#define COL2 BIT0
#define COL3 BIT2

int check_key_pressed();
void init_keypad();

#endif /* KEYPAD_H_ */
