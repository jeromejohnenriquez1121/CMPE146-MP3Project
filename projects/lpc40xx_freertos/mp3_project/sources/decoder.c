#include "FreeRTOS.h"

#include "LCD.h"
#include "decoder.h"
#include "delay.h"
#include "ff.h"
#include "lpc40xx.h"
#include "mp3_functions.h"
#include "queue.h"
#include "semphr.h"
#include "song_list.h"
#include "ssp0_mp3.h"
#include <stdbool.h>
#include <stdio.h>

#define DEBUG_ENABLE 1

QueueHandle_t pause_q;
QueueHandle_t play_q;

static uint32_t delay_time = 100;

/*********************************************************************************************************/
//                                          Public Functions
/*********************************************************************************************************/

void decoder__initialize(uint32_t max_clock_as_mhz) {
  gpio__reset(gpio_reset_pin);
  delay__ms(delay_time);
  gpio__set(gpio_reset_pin);
  delay__ms(delay_time);
  printf("a\n");

  ssp0_mp3__init(max_clock_as_mhz);
  delay__ms(delay_time);
  printf("a\n");

  decoder__set_pins();
  printf("a\n");

  decoder__send_to_sci(sci_mode, 0x48, 0x00);
  printf("a\n");

  decoder__send_to_sci(sci_clock_freq, 0x60, 0x00);
  printf("a\n");

  mp3_functions__init_volume();
  printf("a\n");

  mp3_functions__init_mode();
  printf("a\n");

  // lcd__init();

#if DEBUG_ENABLE
  decoder__get_status();
#endif
}

void decoder__set_pins(void) {
  /*--- Decoder's pin follows the format: peripheral_name_pin ---*/

  // SPI pins
  spi_sclk_pin = gpio__construct_with_function(0, 15, GPIO__FUNCTION_2);
  spi_mosi_pin = gpio__construct_with_function(0, 18, GPIO__FUNCTION_2);
  spi_miso_pin = gpio__construct_with_function(0, 17, GPIO__FUNCTION_2);

  // GPIO pins
  gpio_reset_pin =
      gpio__construct_with_function(0, 16, GPIO__FUNCITON_0_IO_PIN);
  gpio_xdcs_pin = gpio__construct_with_function(2, 9, GPIO__FUNCITON_0_IO_PIN);
  gpio_xcs_pin = gpio__construct_with_function(2, 7, GPIO__FUNCITON_0_IO_PIN);
  gpio_dreq_pin = gpio__construct_with_function(2, 8, GPIO__FUNCITON_0_IO_PIN);

  gpio_up_button = gpio__construct_with_function(2, 0, GPIO__FUNCITON_0_IO_PIN);
  gpio_down_button =
      gpio__construct_with_function(2, 1, GPIO__FUNCITON_0_IO_PIN);
  gpio_mode_button =
      gpio__construct_with_function(2, 2, GPIO__FUNCITON_0_IO_PIN);

  // Set GPIO as output
  gpio__set_as_output(gpio_reset_pin);
  gpio__set_as_output(gpio_xcs_pin);
  gpio__set_as_output(gpio_xdcs_pin);

  // Set GPIO as input
  gpio__set_as_input(gpio_dreq_pin);
  gpio__set_as_input(gpio_up_button);
  gpio__set_as_input(gpio_down_button);
  gpio__set_as_input(gpio_mode_button);

  // Set GPIO output
  gpio__set(gpio_xdcs_pin);
  gpio__set(gpio_xcs_pin);
  gpio__set(gpio_reset_pin);
}

//----------------------- SCI and SDI Functions -------------------------- //

void decoder__send_to_sci(uint8_t address, uint8_t high_byte,
                          uint8_t low_byte) {
  decoder__set_xcs();
  ssp0_mp3__send_byte(write_opcode);
  ssp0_mp3__send_byte(address);
  ssp0_mp3__send_byte(high_byte);
  ssp0_mp3__send_byte(low_byte);

  while (!decoder__data_ready()) {
    ;
  }
  decoder__clear_xcs();
}

uint16_t decoder__read_from_sci(uint8_t address) {
  const uint8_t dummy_code = 0xFF;
  uint8_t high_byte = 0x0;
  uint8_t low_byte = 0x0;

  decoder__set_xcs();
  ssp0_mp3__send_byte(read_opcode);
  ssp0_mp3__send_byte(address);
  high_byte = ssp0_mp3__send_byte(dummy_code);
  low_byte = ssp0_mp3__send_byte(dummy_code);

  while (!decoder__data_ready()) {
    ;
  }

  decoder__clear_xcs();

  uint16_t result = low_byte;
  result |= (high_byte << 8);

  return result;
}

void decoder__send_to_sdi(uint8_t byte_to_transfer) {

  decoder__set_xdcs();
  ssp0_mp3__send_byte(byte_to_transfer);
  decoder__clear_xdcs();

  while (!decoder__data_ready()) {
    ;
  }
}

//----------------------- Play and Pauses Functions --------------------------
////

void decoder__pause(bool *pause_var) { mp3_functions__enable_pause(pause_var); }
void decoder__play(bool *pause_var) { mp3_functions__disable_pause(pause_var); }

//----------------------- Volume Functions -------------------------- //
void decoder__raise_volume(void) {
  bool result = mp3_functions__raise_volume();

#if DEBUG_ENABLE
  if (result == true)
    printf("Raise volume SUCCESS: %cv %04x.\n",
           mp3_functions__get_current_volume(),
           decoder__read_from_sci(sci_volume_ctl));
  else
    printf("Raise volume FAIL.\n");
#endif
}

void decoder__lower_volume(void) {
  bool result = mp3_functions__lower_volume();

#if DEBUG_ENABLE
  if (result == true)
    printf("Lower volume SUCCESS: %cv %04x.\n",
           mp3_functions__get_current_volume(),
           decoder__read_from_sci(sci_volume_ctl));
  else
    printf("Lower volume FAIL.\n");
#endif
}

void decoder__change_mode(void) {
  mp3_functions__scroll_through_modes();

#if DEBUG_ENABLE
  printf("MP3 mode: %x.\n", current_mode);
#endif
}

//----------------------- Pin Functions -------------------------- //

// RESET
void decoder__set_reset(void) { gpio__reset(gpio_reset_pin); } // Set reset to 0
void decoder__clear_reset(void) { gpio__set(gpio_reset_pin); } // Set reset to 1

// XCS
void decoder__set_xcs(void) { gpio__reset(gpio_xcs_pin); } // Set xcs to 0
void decoder__clear_xcs(void) { gpio__set(gpio_xcs_pin); } // Set xcs to 1

// XDCS
void decoder__set_xdcs(void) { gpio__reset(gpio_xdcs_pin); } // Set xdcs to 0
void decoder__clear_xdcs(void) { gpio__set(gpio_xdcs_pin); } // Set xdcs to 1

// DREQ
bool decoder__data_ready(void) {
  return gpio__get(gpio_dreq_pin); // Gets input from dreq
}

void decoder__get_status(void) {
  uint16_t mode_reading = decoder__read_from_sci(sci_mode);
  uint16_t status_reading = decoder__read_from_sci(sci_status);
  uint16_t clock_freq_reading = decoder__read_from_sci(sci_clock_freq);
  uint16_t volume_reading = decoder__read_from_sci(sci_volume_ctl);
  printf("Mode: %04x.\n", mode_reading);
  printf("Status: %04x.\n", status_reading);
  printf("Clock frequency: %04x.\n", clock_freq_reading);
  printf("Volume: %04x.\n", volume_reading);
}

//----------------------- Bass and Treble Level Functions
//-------------------------- //

void decoder__raise_bass(void) {
  mp3_functions__raise_bass();

#if DEBUG_ENABLE
  printf("Bass Register: %04x\n.", decoder__read_from_sci(sci_bass));
#endif
}
void decoder__lower_bass(void) {
  mp3_functions__lower_bass();

#if DEBUG_ENABLE
  printf("Bass Register: %04x\n.", decoder__read_from_sci(sci_bass));
#endif
}
void decoder__raise_treble(void) {
  mp3_functions__raise_treble();

#if DEBUG_ENABLE
  printf("Bass Register: %04x\n.", decoder__read_from_sci(sci_bass));
#endif
}
void decoder__lower_treble(void) {
  mp3_functions__lower_treble();

#if DEBUG_ENABLE
  printf("Bass Register: %04x\n.", decoder__read_from_sci(sci_bass));
#endif
}
