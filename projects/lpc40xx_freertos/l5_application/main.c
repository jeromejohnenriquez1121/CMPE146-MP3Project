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

QueueHandle_t name_queue;
QueueHandle_t data_queue;

char song_name[32];
char song_data[512];

FIL file;
FRESULT fresult;
UINT bytes_read;

static int not_eof(void);
static bool is_eof(void);

static bool read_file(void);
static bool close_file(void);
static bool open_file(void);
static void initialize_queues(void);

/*********************************************************************************************************/
//                                          Public Functions //
/*********************************************************************************************************/

void reader_task(void *parameter) {
  while (1) {
    char temp_name[32] = "";

    printf("Waiting to receive song.\n");
    xQueueReceive(name_queue, &temp_name[0], portMAX_DELAY);

    strcpy(song_name, temp_name);
    printf("Received %s.\n", song_name);

    if (open_file()) {

      while (not_eof()) {
        read_file();

        if (is_eof()) {
          close_file();
          printf("Closed file.\n");
          break;
        }

        xQueueSend(data_queue, &song_data[0], portMAX_DELAY);
        printf("Sent to data queue.\n");
        memset(song_data, 0, sizeof(song_data));
      }
    } else {
      printf("File cannot be opened.\n");
    }
  }
}

void player_task(void *parameter) {

  while (1) {
    xQueueReceive(data_queue, &song_data[0], portMAX_DELAY);

    for (int i = 0; i < sizeof(song_data); i++) {
      decoder__send_to_sdi(song_data[i]);
      printf("Sent to decoder byte[%d].\n", i);
    }
  }
}

int main(void) {
  sj2_cli__init();

  decoder__initialize();
  printf("Initialized decoder.\n");

  decoder__set_pins();
  printf("Set pins.\n");
  decoder__get_status();

  initialize_queues();
  printf("Initialized pins.\n");

  xTaskCreate(reader_task, "Reads file from SD card", 4096 / sizeof(void *),
              NULL, PRIORITY_HIGH, NULL);
  xTaskCreate(player_task, "Sends files to decoder", 4096 / sizeof(void *),
              NULL, PRIORITY_HIGH, NULL);

  vTaskStartScheduler();

  return 1;
}

/*********************************************************************************************************/
//                                          Private Functions //
/*********************************************************************************************************/

static int not_eof(void) { return 1; }

static bool is_eof(void) { return bytes_read == 0; }

static bool open_file(void) {
  fresult = f_open(&file, &song_name[0], FA_READ);
  return fresult == FR_OK;
}

static bool read_file(void) {
  fresult = f_read(&file, &song_data[0], sizeof(song_data), &bytes_read);
  return fresult == FR_OK;
}

static bool close_file(void) {
  fresult = f_close(&file);
  return fresult == FR_OK;
}

static void initialize_queues(void) {
  name_queue = xQueueCreate(1, sizeof(song_name));
  data_queue = xQueueCreate(1, sizeof(song_data));
}