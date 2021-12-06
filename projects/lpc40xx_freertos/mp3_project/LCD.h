#pragma once

// intialization
void backlight(int x);
void setBaud();
void sj2_buttons();
void setTX();

// tests
// void LCD_task(void);
void producer_task();

// tasks
void menu(void *parameter);
void UARTprintFromQueue(void *parameter);
void UARTprint(char);

// common screen commands
void clearScreen();
void Turn_blinkingCursor();
void Set_Cursor_second_line();
void Set_Cursor_first_line();
void Set_Cursor_End_first_line();

typedef enum {
  command_word1 = 0xFE,
  command_word2 = 0x7C,
  lowest_brightness = 0x80,
  highest_brightness = 0x9D,
  Baud_9600 = 0x0D,
  First_Line = 0x80,
  Second_Line = 0XC0,
  Clear_Screen = 0x01,
  Blinking_Cursor = 0x0D,
  End_of_First_Line = 0x8F

} special_commands_e;