#include "decoder.h"
#include "delay.h"
#include "ssp0_mp3.h"
#include <stdbool.h>
#include <stdio.h>

/*********************************************************************************************************/
//                                             Declarations
/*********************************************************************************************************/

static uint32_t delay_time = 100;

/*********************************************************************************************************/
//                                          Public Functions
/*********************************************************************************************************/

void decoder__initialize(uint32_t max_clock_as_mhz) {
  gpio__reset(gpio_reset_pin);
  delay__ms(delay_time);
  gpio__set(gpio_reset_pin);
  delay__ms(delay_time);

  ssp0_mp3__init(max_clock_as_mhz);
  delay__ms(delay_time);

  decoder__send_to_sci(mode, 0x48, 0x00);
  delay__ms(delay_time);

  decoder__send_to_sci(clock_freq, 0x60, 0x00);
  delay__ms(delay_time);
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

  // Set GPIO pin direction
  gpio__set_as_output(gpio_reset_pin);
  gpio__set_as_output(gpio_xcs_pin);
  gpio__set_as_output(gpio_xdcs_pin);
  gpio__set_as_input(gpio_dreq_pin);

  // Set GPIO output
  gpio__set(gpio_xdcs_pin);
  gpio__set(gpio_xcs_pin);
  gpio__set(gpio_reset_pin);
}

void decoder__send_to_sci(uint8_t address, uint8_t high_byte,
                          uint8_t low_byte) {

  while (!decoder__data_ready()) {
    ;
  }

  decoder__set_xcs();
  ssp0_mp3__send_byte(write_opcode);

  decoder__clear_xcs();

  while (!decoder__data_ready()) {
    ;
  }
}

uint16_t decoder__read_from_sci(uint8_t address) {
  const uint8_t dummy_code = 0xFF;
  uint8_t high_byte = 0x0;
  uint8_t low_byte = 0x0;

  while (!decoder__data_ready()) {
    ;
  }

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
  uint16_t mode_reading = decoder__read_from_sci(mode);
  uint16_t status_reading = decoder__read_from_sci(sci_status);
  printf("Mode: %04x.\n", mode_reading);
  printf("Status: %04x.\n", status_reading);
}
