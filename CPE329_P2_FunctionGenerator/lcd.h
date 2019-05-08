#ifndef LCD_H_
#define LCD_H_

#define D04 BIT4
#define D05 BIT5
#define D06 BIT6
#define D07 BIT7

#define RS BIT5
#define RW BIT6
#define E  BIT7

#define LCD_CONTROL P3
#define LCD_DATA P4

#define FIRST_FUNCTION_SET 0x30
#define FUNCTION_SET 0x28
#define DISPLAY_ON 0x0F
#define DISPLAY_CLEAR 0x01
#define ENTRY_MODE_SET 0x07
#define FIRST_ROW 0x80
#define SECOND_ROW 0xC0

void init_LCD();
void write_char_LCD(char character);
void write_string(char* string);
void change_line();
void clear_LCD();
void home_LCD();

#endif /* LCD_H_ */
