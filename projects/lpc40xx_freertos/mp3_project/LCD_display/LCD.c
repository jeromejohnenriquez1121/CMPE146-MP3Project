
#include <stdbool.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "board_io.h"
#include "common_macros.h"
#include "ff.h"
#include "periodic_scheduler.h"
#include "queue.h"

#include "sj2_cli.h"
#include "task.h"
#include "uart.h"

#include "LCD.h"
#include "gpio_lab.h"
#include "song_list.h"

QueueHandle_t queue_receive;
QueueHandle_t queue_transmit;

QueueHandle_t up_queue;
QueueHandle_t down_queue;
QueueHandle_t selectB_queue;

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

void backlight_max() {
  uint32_t backLight = 0;
  backLight = ((backLight >> 0) & 0x00);
  backLight = ((backLight >> 8) & 0x00);

  uart__polled_put(UART__2, ((backLight >> 0) | command_word2));
  uart__polled_put(UART__2, ((backLight >> 8) | highest_brightness));
}

void setBaud() {
  uint32_t baud = 0;
  baud = ((baud >> 0) & 0x00);
  baud = ((baud >> 8) & 0x00);

  uart__polled_put(UART__2, ((baud >> 0) | command_word2));
  uart__polled_put(UART__2, ((baud >> 0) | Baud_9600));
}

void button_task() {
  sj2_buttons();
  up_queue = xQueueCreate(2, sizeof(bool));
  down_queue = xQueueCreate(2, sizeof(bool));
  selectB_queue = xQueueCreate(2, sizeof(bool));

  bool pass = true;

  while (1) {
    if (gpio0__get_level(upN)) {
      xQueueSend(up_queue, &pass, 0);
    }
    if (gpio0__get_level(downN)) {
      xQueueSend(down_queue, &pass, 0);
    }
    if (gpio1__get_level(selectN)) {
      xQueueSend(selectB_queue, &pass, 0);
    }
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

  button_task();

  while (1) {

    xQueueReceive(up_queue, &out, portMAX_DELAY);
    uart__polled_put(UART__2, out);
  }
}

void UARTprint(char a) { uart__polled_put(UART__2, a); }

void sj2_buttons() {
  gpio__construct_as_input(0, 29); // temp for testing
  gpio__construct_as_input(0, 30);
  gpio__construct_as_input(1, 19);
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
  song_list__populate();
  // backlight_max();
  /*
  volatile bool downB = gpio0__get_level(downN);
  volatile bool upB = gpio0__get_level(upN);
  volatile bool selectB = gpio1__get_level(selectN);
  */
  setTX();

  queue_receive = xQueueCreate(100, sizeof(uint32_t));
  queue_transmit = xQueueCreate(100, sizeof(uint32_t));

  uart__init(UART__2, (96 * 1000 * 1000), 9600);

  uart__enable_queues(UART__2, queue_receive, queue_transmit);

  Turn_blinkingCursor();

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
        if (gpio0__get_level(downN)) {
          vTaskDelay(500);
          menu++;
        }

        else if (gpio0__get_level(upN)) {
          vTaskDelay(500);
          menu = 2;
        } else if (gpio1__get_level(selectN)) {
          play_select();
          break;
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
        if (gpio0__get_level(downN)) {

          vTaskDelay(500);
          menu++;
        }

        else if (gpio0__get_level(upN)) {
          vTaskDelay(500);
          menu--;
        } else if (gpio1__get_level(selectN)) {
          list_select();
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
void play_select() {
  /* return later

  int count = 0;
   int number_of_songs = song_list__get_item_count();

  while (count < number_of_songs-2)
  {
  play_func(count);
  count = count+2;
  }

  */
}

void options_select() {
  char volume[] = "Volume";
  char bass[] = "Bass";
  char treble[] = "Treble";

  int mode = 0;

  while (1) {
    clearScreen();
    Set_Cursor_first_line();

    if (mode == 0) {

      for (int i = 0; i < sizeof(volume) - 1; i++) 
	  {
        UARTprint(volume[i]);
      }
      UARTprint(':');
      /*return later
              UARTprint(get value for volume() )


              if(gpio(downN))
              {
                      set vlue for volume -1
              }
      if(gpio(upN))
      {
                set vlue for volume +1
      }
              if(gpio(selectB))
              {
                      mode++;
              }
      */
    } 
	else if (mode == 1) {
      for (int i = 0; i < sizeof(bass) - 1; i++) {
        UARTprint(bass[i]);
      }
      UARTprint(':');
      /*return later
                  UARTprint(get value for bass() )


                  if(gpio(downN))
                  {
                                  set value for bass -1
                  }
                  if(gpio(upN))
                  {
                                          set value for bass +1
                  }
                  if(gpio(selectB))
                  {
                                  mode++;
                  }
       */
    }

    else if (mode == 2) {
      for (int i = 0; i < sizeof(treble) - 1; i++) {
        UARTprint(treble[i]);
      }
      UARTprint(':');
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

void list_select() {

  clearScreen();
  int count = 0;
  int number_of_songs = song_list__get_item_count();

  while (1) {
    clearScreen();
    Set_Cursor_first_line();
    print_song(count);
    Set_Cursor_End_first_line();

    fprintf(stderr, "%d song:", count / 2);
    fprintf(stderr, song_list__get_name_for_item(count));
    fprintf(stderr, "\n");

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
		  /*return later
          play_func(count);
          */
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

void print_song(int x) {
  char *songname;
  songname = song_list__get_name_for_item(x);

  for (int i = 0; i < 31; i++) {
    if (songname[i] != '.') {
      UARTprint(songname[i]);
    }
    if (songname[i] == '.') {
      break;
    }
  }
}
