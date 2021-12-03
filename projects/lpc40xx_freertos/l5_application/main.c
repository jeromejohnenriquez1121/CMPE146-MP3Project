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
//                              Declarations
/**************************************************************************/

QueueHandle_t song_name_queue;
QueueHandle_t song_data_queue;

FIL file;
FRESULT fresult;
UINT bytes_read;

static int not_eof(void);
static bool is_eof(void);
static uint32_t delay_time = 100;

static bool read_file(char *song_data);
static bool close_file(void);
static bool open_file(char *song_name);
static void initialize_queues(void);

/**************************************************************************/
//                            Public Functions
/**************************************************************************/

void reader_task(void *parameter) {
  char song_name[32];
  char song_data[512];

  while (1) {
    xQueueReceive(song_name_queue, &song_name[0], portMAX_DELAY);
    printf("Received song: %s.\n", song_name);

    if (f_open(&file, &song_name[0], FA_READ) == FR_OK) {
      printf("Opened file.\n");
      delay__ms(delay_time);
      while (not_eof()) {
        f_read(&file, &song_data[0], sizeof(song_data), &bytes_read);
        if (is_eof()) {
          f_close(&file);
          printf("Closed file.\n");
          break;
        } else {
          xQueueSend(song_data_queue, &song_data[0], portMAX_DELAY);
          printf("Sent to data queue.\n");
        }
      }
    } else {
      printf("File cannot be opened.\n");
    }
  }
}

void player_task(void *parameter) {
  char song_data[512];

  while (1) {
    if (xQueueReceive(song_data_queue, &song_data[0], portMAX_DELAY)) {
      printf("Received from data queue.\n");
      for (int i = 0; i < 512; i++) {
        while (!decoder__data_ready()) {
          ;
        }

        decoder__send_to_sdi(song_data[i]);
      }
    }
  }

  // char song_data[512];
  // while (1) {
  //   if (xQueueReceive(song_data_queue, &song_data[0], portMAX_DELAY)) {
  //     for (int i = 0; i < sizeof(song_data); i++) {
  //       printf("%c", song_data[i]);
  //     }
  //   }
  // }
}

int main(void) {
  sj2_cli__init();

  decoder__set_pins();
  printf("Set pins.\n");

  decoder__initialize();
  printf("Initialized decoder.\n");

  initialize_queues();
  delay__ms(100);

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

static bool open_file(char *song_name) {
  fresult = f_open(&file, song_name, FA_READ);
  return fresult == FR_OK;
}

static bool read_file(char *song_data) {
  fresult = f_read(&file, *song_data, sizeof(*song_data), &bytes_read);
  return fresult == FR_OK;
}

static bool close_file(void) {
  fresult = f_close(&file);
  return fresult == FR_OK;
}

// -------------------------- Queue Functions ----------------------------- //
static void initialize_queues(void) {
  song_name_queue = xQueueCreate(1, 32);
  song_data_queue = xQueueCreate(1, 512);
}