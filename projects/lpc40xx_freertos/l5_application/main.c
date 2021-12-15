#include <stdbool.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include "LCD.h"
#include "board_io.h"
#include "common_macros.h"
#include "decoder.h"
#include "delay.h"
#include "ff.h"
#include "gpio.h"
#include "mp3_functions.h"
#include "periodic_scheduler.h"
#include "queue.h"
#include "sj2_cli.h"
#include "song_list.h"
#include "ssp0_mp3.h"
#include "string.h"

QueueHandle_t song_name_queue;
QueueHandle_t song_data_queue;
QueueHandle_t pause_q;
QueueHandle_t play_q;

QueueHandle_t button_control_q;

bool is_paused;
bool is_skip;
bool is_rewind;

FIL file;
FRESULT fresult;
UINT bytes_read;

static int not_eof(void);
static bool is_eof(void);
static uint32_t delay_time = 100;
static uint32_t spi_clock_freq_in_mhz = 1;
size_t song_list_index;

static void initialize_queues(void);
static void rewind_song(void);
static void skip_song(void);

/**************************************************************************/
//                            Public Functions
/**************************************************************************/

void reader_task(void *parameter) {

  char song_name[32];
  char song_data[512];

  while (1) {
    xQueueReceive(song_name_queue, &song_name[0], portMAX_DELAY);

    lcd__print_song_string(&song_name[0]);

    if (f_open(&file, &song_name[0], FA_READ) == FR_OK) {
      while (not_eof()) {
        f_read(&file, &song_data[0], sizeof(song_data), &bytes_read);
        if (is_eof()) {
          f_close(&file);
          skip_song();
          break;
        } else {
          while (is_paused) {
            vTaskDelay(5);
          }
          xQueueSend(song_data_queue, &song_data[0], portMAX_DELAY);
        }
      }
    }
  }
}

void player_task(void *parameter) {
  char song_data[512];

  while (1) {
    if (xQueueReceive(song_data_queue, &song_data[0], portMAX_DELAY)) {
      for (int i = 0; i < sizeof(song_data); i++) {
        while (!decoder__data_ready()) {
          ;
        }

        decoder__send_to_sdi(song_data[i]);
      }
    }
  }
}

void detect_input_buttons(void *parameter) {
  uint8_t up_button_action = 1;
  uint8_t down_button_action = 2;
  uint8_t mode_action = 3;

  while (1) {
    if (!gpio__get(gpio_up_button)) {
      while (!gpio__get(gpio_up_button)) {
        ;
      }
      xQueueSend(button_control_q, &up_button_action, 0);
    }
    if (!gpio__get(gpio_down_button)) {
      while (!gpio__get(gpio_down_button)) {
        ;
      }
      xQueueSend(button_control_q, &down_button_action, 0);
    }
    if (!gpio__get(gpio_mode_button)) {
      while (!gpio__get(gpio_mode_button)) {
        ;
      }
      xQueueSend(button_control_q, &mode_action, 0);
    }
  }
}

void button_controls(void *parameter) {
  uint8_t button_action;

  while (1) {
    button_action = 0;
    if (xQueueReceive(button_control_q, &button_action, portMAX_DELAY)) {
      if (button_action == 1) {
        if (current_mode == pause_mode) {
          decoder__play(&is_paused);
        }
        if (current_mode == volume_mode) {
          decoder__raise_volume();
        }
        if (current_mode == bass_mode) {
          decoder__raise_bass();
        }
        if (current_mode == treble_mode) {
          decoder__raise_treble();
        }
        if (current_mode == rewind_skip_mode) {

          rewind_song();
        }
      }
      if (button_action == 2) {
        if (current_mode == pause_mode) {
          if (!is_paused) {
            decoder__pause(&is_paused);
          }
        }
        if (current_mode == volume_mode) {
          decoder__lower_volume();
        }
        if (current_mode == bass_mode) {
          decoder__lower_bass();
        }
        if (current_mode == treble_mode) {
          decoder__lower_treble();
        }
        if (current_mode == rewind_skip_mode) {

          skip_song();
        }
      }
      if (button_action == 3) {
        decoder__change_mode();
      }
    }
  }
}

int main(void) {
  sj2_cli__init();
  printf("a\n");

  song_list_index = 0;

  song_list__populate();
  printf("a\n");

  char *first_song = song_list__get_name_for_item(0);

  initialize_queues();
  printf("a\n");

  xQueueSend(song_name_queue, &first_song[0], 0);
  printf("a\n");

  decoder__initialize(spi_clock_freq_in_mhz);
  printf("a\n");

  delay__ms(delay_time);

  is_paused = true;
  is_skip = false;
  is_rewind = false;

  xTaskCreate(reader_task, "Reads file from SD card", 4096 / sizeof(void *),
              NULL, PRIORITY_LOW, NULL);
  xTaskCreate(player_task, "Sends files to decoder", 4096 / sizeof(void *),
              NULL, PRIORITY_HIGH, NULL);
  xTaskCreate(detect_input_buttons, "Detects button pressed",
              4096 / sizeof(void *), NULL, PRIORITY_LOW, NULL);
  xTaskCreate(button_controls, "Play button functions", 4096 / sizeof(void *),
              NULL, PRIORITY_LOW, NULL);
  // xTaskCreate(lcd__menu_task, "Shows menu", 4096 / sizeof(void *), NULL,
  //             PRIORITY_LOW, NULL);
  // xTaskCreate(lcd__uart_print_from_queue, "Shows menu", 4096 / sizeof(void
  // *),
  //             NULL, PRIORITY_LOW, NULL);

  vTaskStartScheduler();

  return 1; // Should never halt
}

/**************************************************************************/
//                            Private Functions
/**************************************************************************/

// ------------------------- File I/O Functions -------------------------- //
static int not_eof(void) { return 1; }

static bool is_eof(void) { return bytes_read == 0; }

// -------------------------- Queue Functions ----------------------------- //
static void initialize_queues(void) {
  song_name_queue = xQueueCreate(1, 32);
  song_data_queue = xQueueCreate(1, 512);
  button_control_q = xQueueCreate(1, sizeof(uint8_t));
  pause_q = xQueueCreate(1, sizeof(uint8_t));
  play_q = xQueueCreate(1, sizeof(bool));
}

// -------------------------- Rewind / Skip Functions -------------------------
// //

static void rewind_song(void) {
  if (song_list_index > 0) {
    bytes_read = 0;
    f_close(&file);
    song_list_index = song_list_index - 2;
    char *prev_song = song_list__get_name_for_item(song_list_index);

    xQueueSend(song_name_queue, &prev_song[0], 0);
  }
}
static void skip_song(void) {
  size_t list_size = song_list__get_item_count();

  if (song_list_index < list_size - 1) {
    bytes_read = 0;
    f_close(&file);
    song_list_index = song_list_index + 2;
    char *next_song = song_list__get_name_for_item(song_list_index);

    xQueueSend(song_name_queue, &next_song[0], 0);
  }
}
