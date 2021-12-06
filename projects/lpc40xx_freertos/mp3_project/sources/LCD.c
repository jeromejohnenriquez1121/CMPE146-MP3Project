
#include <stdbool.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "board_io.h"
#include "common_macros.h"
#include "ff.h"
#include "periodic_scheduler.h"
#include "queue.h"

#include "gpio_lab.h"
#include "sj2_cli.h"
#include "task.h"
#include "uart.h"

#include "LCD.h"
#include "song_list.h"

QueueHandle_t queue_receive;
QueueHandle_t queue_transmit;

QueueHandle_t LCD_queue;

void clearScreen() {

  uint32_t clear_screen = 0;

  clear_screen = ((clear_screen >> 8) & 0xff);
  clear_screen = ((clear_screen >> 0) & 0xff);

  uart__polled_put(UART__2, ((clear_screen >> 0) | command_word1));
  uart__polled_put(UART__2, ((clear_screen >> 8) | Clear_Screen));
}

void backlight(int x) {

  uint32_t backLight = 0;
  backLight = ((backLight >> 0) & 0x00);
  backLight = ((backLight >> 8) & 0x00);

  if (x > 0) {
    uart__polled_put(UART__2, ((backLight >> 0) | command_word2));
    uart__polled_put(UART__2, ((backLight >> 8) | lowest_brightness));
  } else {
    uart__polled_put(UART__2, ((backLight >> 0) | command_word2));
    uart__polled_put(UART__2, ((backLight >> 8) | highest_brightness));
  }
}

void setBaud() {
  uint32_t baud = 0;
  baud = ((baud >> 0) & 0x00);
  baud = ((baud >> 8) & 0x00);

  uart__polled_put(UART__2, ((baud >> 0) | command_word2));
  uart__polled_put(UART__2, ((baud >> 0) | Baud_9600));
}

void producer_task() {
  char string[] = "This is a queue test";

  for (int i = 0; i < sizeof(string) - 1; i++)

  {
    xQueueSend(LCD_queue, &string[i], 100);
  }
}

void UARTprintFromQueue(void *parameter) {

  queue_receive = xQueueCreate(100, sizeof(uint32_t));
  queue_transmit = xQueueCreate(100, sizeof(uint32_t));

  LPC_IOCON->P0_10 &= 0b000;
  LPC_IOCON->P0_10 |= 0b001; // setting pin P0_10 as tx

  uart__init(UART__2, (96 * 1000 * 1000), 9600);

  uart__enable_queues(UART__2, queue_receive, queue_transmit);

  char out;

  clearScreen();

  producer_task();

  while (1) {

    xQueueReceive(LCD_queue, &out, portMAX_DELAY);
    uart__polled_put(UART__2, out);
  }
}

void UARTprint(char a) { uart__polled_put(UART__2, a); }

void sj2_buttons() {
  gpio0__set_as_input(29); // temp for testing
  gpio0__set_as_input(30);
  gpio1__set_as_input(19);
}

void setTX() {
  LPC_IOCON->P0_10 &= 0b000;
  LPC_IOCON->P0_10 |= 0b001; // setting pin P0_10 as tx
}

void Turn_blinkingCursor() {
  uart__polled_put(UART__2, command_word1);
  uart__polled_put(UART__2, Blinking_Cursor);
}

void Set_Cursor_second_line() {
  uart__polled_put(UART__2, command_word1);
  uart__polled_put(UART__2, Second_Line);
}

void Set_Cursor_first_line() {
  uart__polled_put(UART__2, command_word1);
  uart__polled_put(UART__2, First_Line);
}

void Set_Cursor_End_first_line() {
  uart__polled_put(UART__2, command_word1);
  uart__polled_put(UART__2, End_of_First_Line);
}

void menu(void *parameter) {

  sj2_buttons(); // initalize sj2 buttons for testing.

  volatile bool up, down, selectB;
  down = gpio0__get_level(29);
  up = gpio0__get_level(30);
  selectB = gpio1__get_level(19);

  setTX();

  queue_receive = xQueueCreate(100, sizeof(uint32_t));
  queue_transmit = xQueueCreate(100, sizeof(uint32_t));

  uart__init(UART__2, (96 * 1000 * 1000), 9600);

  uart__enable_queues(UART__2, queue_receive, queue_transmit);

  Turn_blinkingCursor();

  int menu = 0;

  char play[] = "Play";
  char list[] = "List";
  char options[] = "Options";

  int playN = 0;
  int listN = 1;
  int option = 2;

  while (1) {

    if (menu == playN) {
      clearScreen();

      for (int i = 0; i < sizeof(play) - 1; i++) {
        UARTprint(play[i]);
      }

      Set_Cursor_second_line();

      for (int i = 0; i < sizeof(list) - 1; i++) {
        UARTprint(list[i]);
      }

      Set_Cursor_End_first_line();

      while (menu == playN) {
        if (down) {
          vTaskDelay(500);
          menu++;
        }

        else if (up) {
          vTaskDelay(500);
          menu = 2;
        } else if (selectB) {
          vTaskDelay(500);
          song_list__populate();
          /*
          size_t song_number = 0 for (song_number; song_number < song_list__get_item_count(); song_number++) {
            printf("Song %2d: %s\n", (1 + song_number), song_list__get_name_for_item(song_number));
          }
                  */
          while (1) {

            int song_number = 0;

            UARTprint(song_list__get_name_for_item(song_number));

            Set_Cursor_second_line();

            UARTprint(song_list__get_name_for_item(song_number + 1));

            Set_Cursor_End_first_line();

            if (down) {
              song_number++;
            }
            if (up) {
              song_number--;
            }

            if (selectB) {
              // send song name over to decoder?

              clearScreen();
              char playing[] = "Playing ";
              for (int i = 0; i < sizeof(playing) - 1; i++) {
                UARTprint(playing[i]);
              }
              UARTprint(song_list__get_name_for_item(song_number));
            }

            if (song_number > song_list__get_item_count() - 1) // scroll past end of list loop around
            {
              UARTprint(song_list__get_name_for_item(song_list__get_item_count));

              Set_Cursor_second_line();

              UARTprint(song_list__get_name_for_item(1));

              Set_Cursor_End_first_line();

              if (down) {
                song_number = 1;
              }
              if (up) {
                song_number--;
              }
            }

            if (song_number < 1) // scroll behind start of list loop around
            {
              UARTprint(song_list__get_name_for_item(song_list__get_item_count));

              Set_Cursor_second_line();

              UARTprint(song_list__get_name_for_item(1));

              Set_Cursor_End_first_line();

              if (gpio0__get_level(29)) {
                song_number = song_list__get_item_count() - 1;
              }
              if (gpio0__get_level(30)) {
                song_number++;
              }
            }
          }
        }
      }
    }

    if (menu == listN) {
      clearScreen();

      for (int i = 0; i < sizeof(list) - 1; i++) {
        UARTprint(list[i]);
      }

      Set_Cursor_second_line();

      for (int i = 0; i < sizeof(options) - 1; i++) {
        UARTprint(options[i]);
      }

      Set_Cursor_End_first_line();

      while (menu == listN) {
        if (down) {

          vTaskDelay(500);
          menu++;
        }

        else if (up) {
          vTaskDelay(500);
          menu--;

        } else if (selectB) {
          vTaskDelay(500);
          song_list__populate();
          /*
          size_t song_number = 0 for (song_number; song_number < song_list__get_item_count(); song_number++) {
            printf("Song %2d: %s\n", (1 + song_number), song_list__get_name_for_item(song_number));
          }
                  */
          while (1) {

            int song_number = 0;

            UARTprint(song_list__get_name_for_item(song_number));

            Set_Cursor_second_line();

            UARTprint(song_list__get_name_for_item(song_number + 1));

            Set_Cursor_End_first_line();

            if (down) {
              song_number++;
            }
            if (up) {
              song_number--;
            }

            if (song_number > song_list__get_item_count() - 1) // scroll past end of list loop around
            {
              UARTprint(song_list__get_name_for_item(song_list__get_item_count));

              Set_Cursor_second_line();

              UARTprint(song_list__get_name_for_item(1));

              Set_Cursor_End_first_line();

              if (down) {
                song_number = 1;
              }
              if (up) {
                song_number--;
              }
            }

            if (song_number < 1) // scroll behind start of list loop around
            {
              UARTprint(song_list__get_name_for_item(song_list__get_item_count));

              Set_Cursor_second_line();

              UARTprint(song_list__get_name_for_item(1));

              Set_Cursor_End_first_line();

              if (gpio0__get_level(29)) {
                song_number = song_list__get_item_count() - 1;
              }
              if (gpio0__get_level(30)) {
                song_number++;
              }
            }
          }
        }
      }
    }

    if (menu == option) {
      clearScreen();

      for (int i = 0; i < sizeof(options) - 1; i++) {
        UARTprint(options[i]);
      }

      Set_Cursor_second_line();

      for (int i = 0; i < sizeof(play) - 1; i++) {
        UARTprint(play[i]);
      }
      Set_Cursor_End_first_line();

      while (menu == option) {
        if (down) {
          vTaskDelay(500);
          menu = 0;
        }

        else if (up) {
          vTaskDelay(500);
          menu--;
        } else if (selectB) {
          char brightness[] = "brightness";
          for (int i = 0; i < sizeof(brightness) - 1; i++) {
            UARTprint(brightness[i]);
            if (selectB) {
              uint8_t tempBright = lowest_brightness;
              while (!selectB) {
                UARTprint(command_word2);
                if (up) {
                  tempBright++;
                  UARTprint(tempBright);
                  vTaskDelay(250);
                } else if (down) {
                  tempBright--;
                  UARTprint(tempBright);
                  vTaskDelay(250);
                }
              }
            }
          }
        }
      }
    }
  }
}
/*
void LCD_task(void) {

  char parameter[] = "This was sent";

  LCD_queue = xQueueCreate(40, sizeof(char));

  // xTaskCreate(UARTprint, "UARTprint", 4096, (void *)parameter, PRIORITY_HIGH, NULL);
  // xTaskCreate(producer_task, "Producer Task", 4096, NULL, PRIORITY_MEDIUM, NULL);

  // xTaskCreate(menu, "Menu", 4096, NULL, PRIORITY_HIGH, NULL);

  // vTaskStartScheduler();

  return 0;
}
*/