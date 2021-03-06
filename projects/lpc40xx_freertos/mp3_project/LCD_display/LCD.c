
#include "LCD.h"
#include "FreeRTOS.h"
#include "clock.h"
#include "decoder.h"
#include "ff.h"
#include "gpio.h"
#include "queue.h"
#include "uart.h"
#include <stdbool.h>
#include <stdio.h>

#include "mp3_functions.h"
#include "song_list.h"

QueueHandle_t queue_receive;
QueueHandle_t queue_transmit;

QueueHandle_t up_queue;
QueueHandle_t down_queue;
QueueHandle_t selectB_queue;

static void set_backlight(int x);
static void set_baud(void);
static void set_tx(void);

#define DEBUG_ENABLE 0

/*********************************************************************************************************/
//                                          Public Functions
/*********************************************************************************************************/
void lcd__init(void) {
  const uint32_t lcd_baud_rate = 9600;
  queue_receive = xQueueCreate(100, sizeof(uint32_t));
  queue_transmit = xQueueCreate(100, sizeof(uint32_t));

  uart__init(UART__2, clock__get_peripheral_clock_hz(), lcd_baud_rate);

  uart__enable_queues(UART__2, queue_receive, queue_transmit);
}

void lcd__clear_screen(void) {
  uart__polled_put(UART__2, command_word1);
  uart__polled_put(UART__2, clear_screen);
}

void lcd__backlight_max(void) {
  uart__polled_put(UART__2, command_word2);
  uart__polled_put(UART__2, highest_brightness);
}

void lcd__uart_print_from_queue_task(void *parameter) {
  char out;
  lcd__clear_screen();

  while (1) {
    xQueueReceive(up_queue, &out, portMAX_DELAY);
    uart__polled_put(UART__2, out);
  }
}

void lcd__uart_print(char symbol) { uart__polled_put(UART__2, symbol); }

void lcd__turn_blinking_cursor(void) {
  uart__polled_put(UART__2, command_word1);
  uart__polled_put(UART__2, blinking_cursor);
}

void lcd__set_cursor_second_line(void) {
  uart__polled_put(UART__2, command_word1);
  uart__polled_put(UART__2, second_line);
}

void lcd__set_cursor_first_line(void) {
  uart__polled_put(UART__2, command_word1);
  uart__polled_put(UART__2, first_line);
}

void lcd__set_cursor_end_first_line(void) {
  uart__polled_put(UART__2, command_word1);
  uart__polled_put(UART__2, end_of_first_line);
}

void lcd__menu_task(void *parameter) {
  song_list__populate();

  set_tx();

  lcd__turn_blinking_cursor();

  int menu = 0;
  bool pass;

  char play[] = "Play";
  char list[] = "List";
  char options[] = "Options";

  int playN = 0;
  int listN = 1;
  int option = 2;

  while (1) {

    if (menu == playN) {
      lcd__clear_screen();

      for (int i = 0; i < sizeof(play) - 1; i++) {
        lcd__uart_print(play[i]);
      }

      lcd__set_cursor_second_line();

      for (int i = 0; i < sizeof(list) - 1; i++) {
        lcd__uart_print(list[i]);
      }

      lcd__set_cursor_end_first_line();

      while (menu == playN) {
        if (gpio0__get_level(downN)) {
          vTaskDelay(500);
          menu++;
        }

        else if (gpio0__get_level(upN)) {
          vTaskDelay(500);
          menu = 2;
        } else if (gpio1__get_level(selectN)) {
          lcd__play_select();
          break;
        }
      }
    }

    if (menu == listN) {
      lcd__clear_screen();

      for (int i = 0; i < sizeof(list) - 1; i++) {
        lcd__uart_print(list[i]);
      }

      lcd__set_cursor_second_line();

      for (int i = 0; i < sizeof(options) - 1; i++) {
        lcd__uart_print(options[i]);
      }

      lcd__set_cursor_end_first_line();

      while (menu == listN) {
        if (gpio0__get_level(downN)) {

          vTaskDelay(500);
          menu++;
        }

        else if (gpio0__get_level(upN)) {
          vTaskDelay(500);
          menu--;
        } else if (gpio1__get_level(selectN)) {
          lcd__list_select();
        }
      }
    }

    if (menu == option) {
      lcd__clear_screen();

      for (int i = 0; i < sizeof(options) - 1; i++) {
        lcd__uart_print(options[i]);
      }

      lcd__set_cursor_second_line();

      for (int i = 0; i < sizeof(play) - 1; i++) {
        lcd__uart_print(play[i]);
      }

      lcd__set_cursor_first_line();

      while (menu == option) {
        if (gpio0__get_level(downN)) {
          vTaskDelay(500);
          menu = 0;
        }

        else if (gpio0__get_level(upN)) {
          vTaskDelay(500);
          menu--;
        }
      }
    }
  }
}

void play_select(void) { /* return later */
}

void options_select() {
  char volume[] = {'0', '1', '2',  '3',  '4',  '5',  '6',
                   '7', '8', '9', '10', '11', '12', '13'};
  char bass[] = "Bass";
  char treble[] = "Treble";

  while (1) {
    lcd__clear_screen();
    lcd__set_cursor_first_line();

    if (current_mode == volume_mode) {
      int volume_index = mp3_functions__get_current_volume;
      lcd__uart_print(volume[volume_index]);
      lcd__uart_print(':');

    } else if (mode == 1) {
      for (int i = 0; i < sizeof(bass) - 1; i++) {
        lcd__uart_print(bass[i]);
      }
      lcd__uart_print(':');
      /*return later       */
    }

    else if (mode == 2) {
      for (int i = 0; i < sizeof(treble) - 1; i++) {
        lcd__uart_print(treble[i]);
      }
      lcd__uart_print(':');
      /*return later
      UARTprint(get value for treble() )

      if(gpio(downN))
      {
                      set value for treble -1
      }
      if(gpio(upN))
      {
                              set value for treble +1
      }
      if(gpio(selectB))
      {
                      mode=0;
      }
       */
    }
  }
}

void lcd__list_select(void) {

  lcd__clear_screen();
  int count = 0;
  int number_of_songs = song_list__get_item_count();

  while (1) {
    lcd__clear_screen();
    lcd__set_cursor_first_line();
    lcd__print_song(count);
    lcd__set_cursor_end_first_line();

#if DEBUG_ENABLE
    fprintf(stderr, "%d song:", count / 2);
    fprintf(stderr, song_list__get_name_for_item(count));
    fprintf(stderr, "\n");
#endif

    while (1) {
      if (gpio0__get_level(downN)) {
        vTaskDelay(500);
        count = count + 2;
        break;
      } else if (gpio0__get_level(upN)) {
        vTaskDelay(500);
        count = count - 2;
        break;
      } else if (gpio1__get_level(selectN)) {
        /*return later */
      }
    }

    if (count > number_of_songs - 2) {
      count = 0;
    }

    else if (count < 0) {
      count = number_of_songs - 2;
    }
  }
}

void print_song(int song) {
  char *songname;
  songname = song_list__get_name_for_item(song);

  for (int i = 0; i < 31; i++) {
    if (songname[i] != '.') {
      lcd__uart_print(songname[i]);
    }
    if (songname[i] == '.') {
      break;
    }
  }
}

/*********************************************************************************************************/
//                                          Private Functions
/*********************************************************************************************************/

static void set_backlight(int x) {
  if (x > 0) {
    uart__polled_put(UART__2, command_word2);
    uart__polled_put(UART__2, (lowest_brightness));
  } else {
    uart__polled_put(UART__2, command_word2);
    uart__polled_put(UART__2, highest_brightness);
  }
}
static void set_baud(void) {
  uart__polled_put(UART__2, command_word2);
  uart__polled_put(UART__2, baud_9600);
}
static void set_tx(void) {
  gpio__construct_with_function(0, 10, GPIO__FUNCTION_1);
}
