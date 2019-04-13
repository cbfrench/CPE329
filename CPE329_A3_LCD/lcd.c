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
#include "lcd.h"

void initLCD()
{
    // P3 == Control
    // P4 == Data
    delay_us(6000);

    // Function Set: Empty

    LCD_Control -> OUT &= ~E;           // Set E Low
    LCD_Control -> OUT &= ~(RS | RW);   // Set RS and RW Low
    LCD_Data -> OUT = 0x30;             // Write 00110000 (1st Function Set)

    LCD_Control -> OUT |= E;            // Pulse E
    delay_us(0);
    LCD_Control -> OUT &= ~E;

    delay_us(40);

    // ****************************************************
    // Function Set: Lines & Resolution

    LCD_Control -> OUT &= ~(RS | RW);   // Set RS and RW Low
    LCD_Control -> OUT &= ~E;           // Set E Low
    LCD_Data -> OUT = 0x20;             // Write 0010 0000 (2nd Function Set)

    LCD_Control -> OUT |= E;            // Pulse E
    delay_us(0);
    LCD_Control -> OUT &= ~E;

    LCD_Data -> OUT = 0x80;             // Write 1000 0000  N=1 (2-Line), F=0 (5x8 Res)

    LCD_Control -> OUT |= E;            // Pulse E
    delay_us(0);
    LCD_Control -> OUT &= ~E;

    delay_us(40);

    LCD_Control -> OUT &= ~(RS | RW);   // Set RS and RW Low
    LCD_Control -> OUT &= ~E;           // Set E Low
    LCD_Data -> OUT = 0x20;             // Write 0010 0000 (2nd Function Set)

    LCD_Control -> OUT |= E;            // Pulse E
    delay_us(0);
    LCD_Control -> OUT &= ~E;

    LCD_Data -> OUT = 0x80;             // Write 1000 0000  N=1 (2-Line), F=0 (5x8 Res)

    LCD_Control -> OUT |= E;            // Pulse E
    delay_us(0);
    LCD_Control -> OUT &= ~E;

    delay_us(40);

    // ****************************************************
    // Display On, Cursor, and Blink

    LCD_Control -> OUT &= ~(RS | RW);   // Set RS and RW Low
    LCD_Control -> OUT &= ~E;           // Set E Low
    LCD_Data -> OUT = 0x00;             // Write 0000 0000 (Display Enable Upper Bits)

    LCD_Control -> OUT |= E;            // Pulse E
    delay_us(0);
    LCD_Control -> OUT &= ~E;

    LCD_Data -> OUT = 0xF0;             // Write 1111 0000 D=1 (Disp. On), C=1 (Cursor On), B=1 (Blink On)

    LCD_Control -> OUT |= E;            // Pulse E
    delay_us(0);
    LCD_Control -> OUT &= ~E;

    delay_us(40);

    // ****************************************************
    // Display Clear

    LCD_Control -> OUT &= ~(RS | RW);   // Set RS and RW Low
    LCD_Control -> OUT &= ~E;           // Set E Low
    LCD_Data -> OUT = 0x00;             // Write 0000 0000 (Display Clear Upper Bits)

    LCD_Control -> OUT |= E;            // Pulse E
    delay_us(0);
    LCD_Control -> OUT &= ~E;

    LCD_Data -> OUT = 0x10;             // Write 0001 0000 (Display Clear Lower Bits)

    LCD_Control -> OUT |= E;            // Pulse E
    delay_us(0);
    LCD_Control -> OUT &= ~E;

    delay_us(40);

    // ****************************************************
    // Entry Mode Set

    LCD_Control -> OUT &= ~(RS | RW);   // Set RS and RW Low
    LCD_Control -> OUT &= ~E;           // Set E Low
    LCD_Data -> OUT = 0x00;             // Write 0000 0000 (Entry Mode Upper Bits)

    LCD_Control -> OUT |= E;            // Pulse E
    delay_us(0);
    LCD_Control -> OUT &= ~E;

    LCD_Data -> OUT = 0x70;             // Write 0111 0000 I/D=1, S=1

    LCD_Control -> OUT |= E;            // Pulse E
    delay_us(0);
    LCD_Control -> OUT &= ~E;

    delay_us(40);
}

void Write_char_LCD(char character)
{
    uint8_t upperBits = 0xF0 & character;
    uint8_t lowerBits = 0X0F & character;

    lowerBits <<= 4;

    LCD_Control -> OUT |= RS;           // Set RS High
    LCD_Control -> OUT &= ~(RW | E);    // Set RW and E Low
    LCD_Data -> OUT = upperBits;        // Write Upper Bits

    LCD_Control -> OUT |= E;            // Pulse E
    delay_us(1500);
    LCD_Control -> OUT &= ~E;
    delay_us(1500);

    LCD_Data -> OUT = lowerBits;        // Write Lower Bits

    LCD_Control -> OUT |= E;            // Pulse E
    delay_us(1500);
    LCD_Control -> OUT &= ~E;
    delay_us(1500);
}

void writeString(char* string)
{
    int i;

    for(i = 0; i < strlen(string); i++)
    {
        if(string[i] == '\n' || i > 16) changeLine();

        Write_char_LCD(string[i]);
    }
}

void changeLine()
{
    static int lineChanged = 0;

    if(!lineChanged)
    {
        LCD_Control -> OUT &= ~(RS | RW | E);    // Set RW and E Low
        LCD_Data -> OUT = 0xC0;                  // Write Upper Bits (Row 2)

        LCD_Control -> OUT |= E;                 // Pulse E
        delay_us(1500);
        LCD_Control -> OUT &= ~E;
        delay_us(1500);

        LCD_Data -> OUT = 0x00;                  // Write Lower Bits (Zeros)

        LCD_Control -> OUT |= E;                 // Pulse E
        delay_us(1500);
        LCD_Control -> OUT &= ~E;
        delay_us(1500);
    }
    else
    {
        LCD_Control -> OUT &= ~(RS | RW | E);    // Set RW and E Low
        LCD_Data -> OUT = 0x80;                  // Write Upper Bits (First Row)

        LCD_Control -> OUT |= E;                 // Pulse E
        delay_us(1500);
        LCD_Control -> OUT &= ~E;
        delay_us(1500);

        LCD_Data -> OUT = 0x00;                  // Write Lower Bits (Zeros)

        LCD_Control -> OUT |= E;                 // Pulse E
        delay_us(1500);
        LCD_Control -> OUT &= ~E;
        delay_us(1500);
    }

    lineChanged = ~lineChanged;
}

void Clear_LCD()
{
    // ****************************************************
    // Display Clear

    LCD_Control -> OUT &= ~(RS | RW);   // Set RS and RW Low
    LCD_Control -> OUT &= ~E;           // Set E Low
    LCD_Data -> OUT = 0x00;             // Write 0000 0000 (Display Clear Upper Bits)

    LCD_Control -> OUT |= E;            // Pulse E
    delay_us(1500);
    LCD_Control -> OUT &= ~E;

    LCD_Data -> OUT = 0x10;             // Write 0001 0000 (Display Clear Lower Bits)

    LCD_Control -> OUT |= E;            // Pulse E
    delay_us(1500);
    LCD_Control -> OUT &= ~E;

    delay_us(1500);
}

void Home_LCD()
{
    LCD_Control -> OUT &= ~(RS | RW | E);    // Set RW and E Low
    LCD_Data -> OUT = 0x80;                  // Write Upper Bits (First Row)

    LCD_Control -> OUT |= E;                 // Pulse E
    delay_us(1500);
    LCD_Control -> OUT &= ~E;
    delay_us(1500);

    LCD_Data -> OUT = 0x00;                  // Write Lower Bits (Zeros)

    LCD_Control -> OUT |= E;                 // Pulse E
    delay_us(1500);
    LCD_Control -> OUT &= ~E;
    delay_us(1500);
}











