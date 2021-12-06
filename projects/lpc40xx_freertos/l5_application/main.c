#include <stdbool.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include "board_io.h"
#include "common_macros.h"
#include "decoder.h"
#include "delay.h"
#include "ff.h"
#include "periodic_scheduler.h"
#include "queue.h"
#include "sj2_cli.h"
#include "ssp0_mp3.h"
#include "string.h"

/**************************************************************************/
//                    Variable and Function Declarations
/**************************************************************************/

QueueHandle_t song_name_queue;
QueueHandle_t song_data_queue;

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

int main(void) {
  sj2_cli__init();

  decoder__set_pins();

  decoder__initialize(spi_clock_freq_in_mhz);

  initialize_queues();

  delay__ms(delay_time);

  xTaskCreate(reader_task, "Reads file from SD card", 4096 / sizeof(void *),
              NULL, PRIORITY_LOW, NULL);
  xTaskCreate(player_task, "Sends files to decoder", 4096 / sizeof(void *),
              NULL, PRIORITY_HIGH, NULL);

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
}