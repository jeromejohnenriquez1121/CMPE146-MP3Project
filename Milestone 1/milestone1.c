#include <stdbool.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include "board_io.h"
#include "common_macros.h"
#include "ff.h"
#include "periodic_scheduler.h"
#include "queue.h"
#include "sj2_cli.h"

typedef char songname_t[16];
typedef char songdata_t[512];
static uint16_t bytes_read;

QueueHandle_t Q_songname;
QueueHandle_t Q_songdata;

static void open_file(FIL *file, songname_t *name) { f_open(file, name[0], FA_READ); }
static void read_file(FIL *file, char *bytes) { f_read(file, bytes, 512, bytes_read); }
static void close_file(FIL *this_file) { f_close(this_file); }

// Reader tasks receives song-name over Q_songname to start reading it
void mp3_reader_task(void *p)
{
  songname_t name;
  char bytes_512[512];
  while (1)
  {
    printf("Waiting for song to receive.\n");
    xQueueReceive(Q_songname, &name[0], portMAX_DELAY);
    printf("Received song to play: %s.\n", name);

    FIL *file;
    open_file(&file, &name[0]);
    printf("Opened file.\n");
    while (bytes_read != 0)
    {
      read_file(file, &name[0]);
      xQueueSend(Q_songdata, &bytes_512[0], portMAX_DELAY);
      printf("Sent 512 bytes to queue.\n");
    }
    close_file(file);
    printf("Closed file.\n");
  }
}

// Player task receives song data over Q_songdata to send it to the MP3 decoder
// void mp3_player_task(void *p) {
//   char bytes_512[512];

//   while (1) {
//     xQueueReceive(Q_songdata, &bytes_512[0], portMAX_DELAY);
//     for (int i = 0; i < sizeof(bytes_512); i++) {
//       while (!mp3_decoder_needs_data()) {
//         vTaskDelay(1);
//       }

//       spi_send_to_mp3_decoder(bytes_512[i]);
//     }
//   }
// }

void main(void)
{
  fprintf(stderr, "Starting FreeRTOS.\nStarting SJ2 Client.\nSJ2 Client running.\n");
  sj2_cli__init();
  Q_songname = xQueueCreate(1, 16);
  Q_songdata = xQueueCreate(1, 512);

  xTaskCreate(mp3_reader_task, "Read mp3", 8192 / sizeof(void *), NULL, PRIORITY_HIGH, NULL);
  // xTaskCreate(mp3_player_task, "Play mp3", 4096 / sizeof(void *), NULL, PRIORITY_HIGH, NULL);

  vTaskStartScheduler();
}