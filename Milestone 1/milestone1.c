//main.c

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
#include <string.h>

typedef char songname_t[16];
static UINT bytes_read;
static songname_t song_name;
QueueHandle_t Q_songname;
QueueHandle_t Q_songdata;

static FRESULT fr;

// static FRESULT f_read_from_file(FIL *this_file_struct, char *input_bytes){
//   return f_read(this_file_struct, );
// }

// Reader tasks receives song-name over Q_songname to start reading it
void mp3_reader_task(void *p)
{
  char temp_name[16];
  char bytes_512[512];
  while (1)
  {
    printf("Waiting for song to receive.\n");
    xQueueReceive(Q_songname, &temp_name[0], portMAX_DELAY);
    strcpy(song_name, temp_name);
    printf("Received %s.\n", song_name);

    FIL *file;
    fr = f_open(&file, &song_name[0], FA_READ);
    printf("Opening %s.\n", song_name);
    if (fr == FR_OK)
    {
      UINT byte_count = 0;

      while (1)
      {

        f_read(&file, &bytes_512[0], sizeof(bytes_512), &bytes_read);
        if (bytes_read == 0)
        {
          fr = f_close(&file);
          printf("Closed %s.\n", song_name);
          break;
        }
        printf("Bytes read: %d\n", bytes_read);
        xQueueSend(Q_songdata, &bytes_512[0], portMAX_DELAY);
        printf("%s\n", bytes_512);
        memset(bytes_512, 0, sizeof(bytes_512));
        byte_count += bytes_read;
        printf("Byte count: %d\n", byte_count);
      }
    }
    else
    {
      printf("Cannot open file\n");
    }
  }
}

// Player task receives song data over Q_songdata to send it to the MP3 decoder
// void mp3_player_task(void *p) {
//   char bytes_512[512];

//   while (1) {
//     xQueueReceive(Q_songdata, &bytes_512[0], portMAX_DELAY);
//     for (int i = 0; i < sizeof(bytes_512); i++) {
//       while (mp3_decoder_needs_data) {
//         vTaskDelay(1);
//       }

//       // spi_send_to_mp3_decoder(bytes_512[i]);
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

/*---------------------------------------------------------------------------------------------------*/
// handers_general.c

extern QueueHandle_t Q_songname;

app_cli_status_e cli__play_song(app_cli__argument_t argument, sl_string_s user_input_minus_command_name,
                                app_cli__print_string_function cli_output)
{
  // user_input_minus_command_name is actually a 'char *' pointer type
  // We tell the Queue to copy 32 bytes of songname from this location
  sl_string__erase_first_word(user_input_minus_command_name, ' ');

  xQueueSend(Q_songname, &user_input_minus_command_name, portMAX_DELAY);

  printf("Sent %s\n", user_input_minus_command_name.cstring);
  return APP_CLI_STATUS__SUCCESS;
}
