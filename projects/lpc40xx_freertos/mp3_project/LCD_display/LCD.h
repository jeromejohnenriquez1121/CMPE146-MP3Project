#pragma once

// intialization
void lcd__init(void);

// ----------- freeRTOS tasks --------- //
void lcd__menu_task(void *parameter);
void lcd__uart_print_from_queue(void *parameter);

// ----------- LCD tasks ----------- //
void lcd__uart_print(char symbol);
void lcd__play_select(void);
void lcd__list_select(void);
void lcd__options_select(void);
void lcd__print_song(int song);

// ----------- Screen Commands ----------- //
void lcd__clear_screen(void);
void lcd__turn_blinkingCursor(void);
void lcd__set_cursor_second_line(void);
void lcd__set_cursor_first_line(void);
void lcd__set_cursor_end_first_line(void);

typedef enum {
  command_word1 = 0xFE,
  command_word2 = 0x7C,
  lowest_brightness = 0x80,
  highest_brightness = 0x9D,
  baud_9600 = 0x0D,
  first_line = 0x80,
  second_line = 0XC0,
  clear_screen = 0x01,
  blinking_cursor = 0x0D,
  end_of_first_line = 0x8F,
  down_input = 29,
  up_input = 30,
  select_input = 19

} special_commands_e;
