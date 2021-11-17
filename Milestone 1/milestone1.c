//main.c

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
