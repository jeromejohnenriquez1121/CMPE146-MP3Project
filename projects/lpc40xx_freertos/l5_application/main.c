#include <stdbool.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include "board_io.h"
#include "common_macros.h"
#include "decoder.h"
#include "delay.h"
#include "ff.h"
#include "gpio.h"
#include "periodic_scheduler.h"
#include "queue.h"
#include "sj2_cli.h"
#include "ssp0_mp3.h"
#include "string.h"

QueueHandle_t song_name_queue;
QueueHandle_t song_data_queue;

QueueHandle_t button_control_q;

FIL file;
FRESULT fresult;
UINT bytes_read;

static int not_eof(void);
static bool is_eof(void);
static uint32_t delay_time = 100;
static uint32_t spi_clock_freq_in_mhz = 1;

static void initialize_queues(void);

/**************************************************************************/
//                            Public Functions
/**************************************************************************/

void reader_task(void *parameter) {

  char song_name[32];
  char song_data[512];

  while (1) {
    xQueueReceive(song_name_queue, &song_name[0], portMAX_DELAY);

    if (f_open(&file, &song_name[0], FA_READ) == FR_OK) {
      while (not_eof()) {
        f_read(&file, &song_data[0], sizeof(song_data), &bytes_read);
        if (is_eof()) {
          f_close(&file);
          break;
        } else {
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
  uint8_t raise_volume_action = 1;
  uint8_t lower_volume_action = 2;

  while (1) {
    if (!gpio__get(gpio_up_button)) {
      while (!gpio__get(gpio_up_button)) {
        ;
      }
      xQueueSend(button_control_q, &raise_volume_action, 0);
    }
    if (!gpio__get(gpio_down_button)) {
      while (!gpio__get(gpio_down_button)) {
        ;
      }
      xQueueSend(button_control_q, &lower_volume_action, 0);
    }
  }
}

void button_controls(void *parameter) {
  uint8_t button_action;

  while (1) {
    button_action = 0;
    if (xQueueReceive(button_control_q, &button_action, portMAX_DELAY)) {
      if (button_action == 1) {
        decoder__raise_volume();
      }
      if (button_action == 2) {
        decoder__lower_volume();
      }
    }
  }
}

int main(void) {
  sj2_cli__init();

  decoder__initialize(spi_clock_freq_in_mhz);

  initialize_queues();

  delay__ms(delay_time);

  xTaskCreate(reader_task, "Reads file from SD card", 4096 / sizeof(void *),
              NULL, PRIORITY_LOW, NULL);
  xTaskCreate(player_task, "Sends files to decoder", 4096 / sizeof(void *),
              NULL, PRIORITY_HIGH, NULL);
  xTaskCreate(detect_input_buttons, "Detects button pressed",
              4096 / sizeof(void *), NULL, PRIORITY_LOW, NULL);
  xTaskCreate(button_controls, "Play button functions", 4096 / sizeof(void *),
              NULL, PRIORITY_LOW, NULL);

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
}