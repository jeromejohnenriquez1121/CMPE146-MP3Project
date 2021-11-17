/***************************************************************************************************/
//                                         main.c
/***************************************************************************************************/
#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include "board_io.h"
#include "common_macros.h"
#include "periodic_scheduler.h"
#include "queue.h"
#include "sj2_cli.h"
#include "song.h"

QueueHandle_t data_queue;

void read_task(void *parameter)
{
  while (1)
  {
    wait_to_receive_song();
    send_song_to_player_task();
  }
}

// void send_file_to_mp3_decoder(void *parameter) {}

void main(void)
{
  sj2_cli__init();
  initialize_queues();
  printf("Starting freertos....\n");

  xTaskCreate(read_task, "Reads MP3 from SD card", 4096 / sizeof(void *), NULL, PRIORITY_HIGH, NULL);
  // xTaskCreate(send_file_to_mp3_decoder, "Sends MP3 to MP3 decoder", 4096 / sizeof(void *), NULL, PRIORITY_HIGH,
  // NULL);
  vTaskStartScheduler();
}

/***************************************************************************************************/
//                                         handlers_general.c
/***************************************************************************************************/

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

/***************************************************************************************************/
//                                         song.c
/***************************************************************************************************/

#include "song.h"

/*------------------------------------PRIVATE FUNCTIONS------------------------------------------*/

static bool file_function_succeeded() { return file_result == FR_OK; }

static bool open_file(void)
{
  file_result = f_open(&file_struct, &song_name[0], FA_READ);
  return file_function_succeeded();
}

static bool read_file(void)
{
  file_result = f_read(&file_struct, &song_data[0], sizeof(song_data), &bytes_read);
  return file_function_succeeded();
}

static bool close_file(void)
{
  file_result = f_close(&file_struct);
  return file_function_succeeded();
}

static bool eof(void) { return bytes_read == 0; }

static int not_eof(void) { return 1; }

/*------------------------------------PUBLIC FUNCTIONS------------------------------------------*/

void initialize_queues(void)
{
  name_queue = xQueueCreate(1, sizeof(song_name));
  data_queue = xQueueCreate(1, sizeof(song_data));
}

void wait_to_receive_song(void)
{
  char temp_name[32] = "";

  printf("Waiting to receive song.\n");
  delay__ms(1000);
  xQueueReceive(name_queue, &temp_name[0], portMAX_DELAY);

  strcpy(song_name, temp_name);
  printf("Received %s.\n", song_name);
  delay__ms(1000);
}

void send_song_to_player_task(void)
{

  if (open_file())
  {
    while (not_eof())
    {
      read_file();

      if (eof())
      {
        close_file();
        printf("Closed file.\n");
        delay__ms(1000);
        break;
      }

      printf("Bytes read: %d.\n", bytes_read);
      delay__ms(1000);
      xQueueSend(data_queue, &song_data[0], portMAX_DELAY);
      memset(song_data, 0, sizeof(song_data));
    }
  }
  else
  {
    printf("File not found.\n");
  }
}
