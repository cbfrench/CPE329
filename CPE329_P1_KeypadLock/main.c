#include "msp.h"
#include "delay.h"
#include "lcd.h"
#include "keypad.h"

int check_code(char attempt[]){
    int i;
    char combo[4] = {'0', '7', '1', '6'};
    for(i = 0; i < 4; i++){
        if(attempt[i] != combo[i]){
            return 0;
        }
    }
    return 1;
}

void run_combo_lock(){
    int key;
    int success;
    static int count = 0;
    char attempt[4];

    while(1) {
        clear_LCD();
        home_LCD();
        write_string("LOCKED");
        change_line();
        write_string("ENTER KEY");

        while(1){
            if(count == 4)
            {
                //4 digit code entered
                success = check_code(attempt);
                if(success)
                {
                    //attempt is a match to combination
                    clear_LCD();
                    home_LCD();
                    write_string("UNLOCKED");
                }
                //reset count to 0 after every guess
                count = 0 ;
            }
            key = check_key_pressed();
            if(key != -1)
            {
                //key is pressed
                if(key == '*')
                {
                    success = (success == 1) ? 0 : success;
                    break;
                }
                else if (success != 1)
                {
                    if(count == 0)
                    {
                        clear_LCD();
                        home_LCD();
                    }
                    //write key to screen, store key, and increment count
                    write_char_LCD(key);
                    attempt[count] = (char)key;
                    count++;
                }
            }
            delay_ms(300);
        }
    }
}

void main(void)
{
    WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;

    //set up RS, RW, E for LCD
    P3 -> SEL0 &= ~(RS | RW | E);
    P3 -> SEL1 &= ~(RS | RW | E);
    P3 -> DIR |= RS | RW | E;

    //set up data for LCD
    P4 -> SEL0 &= ~ (D04 | D05 | D06 | D07);
    P4 -> SEL1 &= ~(D04 | D05 | D06 | D07);
    P4 -> DIR |= D04 | D05 | D06 | D07;

    //set up pins for keypad rows
    P4 -> SEL0 &= ~ (ROW1 | ROW2 | ROW3 | ROW4);
    P4 -> SEL1 &= ~(ROW1 | ROW2 | ROW3 | ROW4);
    P4 -> DIR &= ~(ROW1 | ROW2 | ROW3 | ROW4);
    P4 -> REN |= ROW1 | ROW2 | ROW3 | ROW4;
    P4 -> OUT &= ~(ROW1 | ROW2 | ROW3 | ROW4);

    //set up pins for keypad columns
    P5 -> SEL0 &= ~ (COL1 | COL2 | COL3);
    P5 -> SEL1 &= ~(COL1 | COL2 | COL3);
    P5 -> DIR |= COL1 | COL2 | COL3;

    set_DCO(FREQ_24_MHz);

    init_LCD();
    run_combo_lock();
}
