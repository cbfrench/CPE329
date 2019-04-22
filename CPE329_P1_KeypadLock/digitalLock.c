/*
 * digitalLock.c
 *
 *  Created on: Apr 21, 2019
 *      Author: Connor French and Joe Sandoval
 *
 *  Provides all Digital Lock functionality. See associated header file for declarations
 */
#include "digitalLock.h"
#include "delay.h"
#include "lcd.h"
#include "keypad.h"

/**
 * Key Combination Needed for Unlock
 */
const char combo[4] = {'0', '7', '1', '6'};

/**
 * Checks to see if the input character array matches the combination 'combo' global variable.
 * @param attempt[] 4 character array
 * @see combo()
 */
int check_code(char attempt[]){
    int i;
    for(i = 0; i < 4; i++)
        if(attempt[i] != combo[i]) return 0;

    return 1;
}

/**
 * Operational Code running the combination lock functionality
 * @see check_code()
 * @see combo()
 */
void run_combo_lock(){
    int key;                                                //!< Key pressed on Keypad
    int success;                                            //!< Success = 1 on correct code entry
    static int count = 0;                                   //!< Iterator; keeps track of number of key entries
    char attempt[4];                                        //!< Holds set of 4 characters composing current unlock attempt

    while(1) {
        clear_LCD();                                        //!< Write Initial Greeting
        home_LCD();
        write_string("LOCKED");
        change_line();
        write_string("ENTER KEY");

        while(1){
            /********************************************//**
             *  Attempt Unlock - After count = 4
             ***********************************************/
            if(count == 4)
            {
                success = check_code(attempt);              //!< Check attempt array against combo()
                                                            //!< @see check_code()
                if(success)
                {
                    clear_LCD();                            //!< If the attempt was successful, write Unlocked
                    home_LCD();
                    write_string("UNLOCKED");

                    count = 0;
                }
                else
                {
                    clear_LCD();                            //!< If the attempt was not successful, write a warning,
                                                            //!< then return to the Initial Greeting
                    home_LCD();
                    write_string(" **INCORRECT!** ");
                    delay_ms(3000);

                    count = 0;
                    break;
                }
            }

            /********************************************//**
             *  Key-Press Acquisition
             ***********************************************/
            key = check_key_pressed();                      //!< Check if Key was pressed
                                                            //!< @see check_key_pressed()
            if(key != -1)
            {
                if(key == '*')                              //!< If key pressed was '*', cancel this attempt and return
                {                                           //!< to Initial greeting
                    count = 0;
                    success = (success == 1) ? 0 : success;
                    break;
                }
                else if (success != 1)                      //!< If any other key is pressed AND success != 1
                {
                    if(count == 0)                          //!< If this is the first Key Press recorded for this attempt
                    {                                       //!< Reset the LCD to display the collected Key-presses
                        clear_LCD();
                        home_LCD();
                    }
                    write_char_LCD(key);                    //!< Write recorded Key Press to LCD
                                                            //!< @see write_char_LCD

                    attempt[count] = (char)key;             //!< Add recorded Key Press to Attempt Array
                                                            //!< @see attempt()
                    count++;
                }
            }
            delay_ms(150);                                  //!< Delay to compensate for user reaction times
        }
    }
}


