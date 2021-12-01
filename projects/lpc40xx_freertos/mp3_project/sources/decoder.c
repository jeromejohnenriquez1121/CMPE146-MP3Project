#include "decoder.h"
#include "delay.h"
#include "ssp0_mp3.h"
#include <stdbool.h>
#include <stdio.h>

// typedef enum {
//   mode = 0x0,
//   status = 0x1,
//   bass = 0x2,
//   clock_freq = 0x3,
//   decode_time = 0x4,
//   audio_data = 0x5,
//   wram = 0x6,
//   wram_addr = 0x7,
//   hdat0 = 0x8,
//   hdat1 = 0x9,
//   aiaddr = 0xA,
//   volume_ctl = 0xB,
//   aictrl0 = 0xC,
//   aictrl1 = 0xD,
//   aictrl2 = 0xE,
//   aictrl3 = 0xF
//} sci_registers_e;

// gpio_s spi_sclk_pin;
// gpio_s spi_mosi_pin;
// gpio_s spi_miso_pin;
// gpio_s gpio_reset_pin;
// gpio_s gpio_xdcs_pin;
// gpio_s gpio_xcs_pin;
// gpio_s gpio_dreq_pin;

/*********************************************************************************************************/
//                                          Public Functions //
/*********************************************************************************************************/

void decoder__initialize(void) {
  decoder__set_reset();
  delay__ms(100);
  decoder__clear_reset();

  ssp0_mp3__init();
  ssp0_mp3__set_prescalar_registers(1);

  // Default mode register: 0x4800
  // Used later to set clock freq, bass and treble settings, volume settings,
  // etc
  decoder__send_to_sci(clock_freq, 0x60, 0x00);
  // decoder__send_to_sci(audio_data, 0xAC, 0x45);
  decoder__send_to_sci(volume_ctl, 0x00, 0x00);

  // ssp2_mp3__set_prescalar_registers(6);
}

void decoder__set_pins(void) {
  spi_sclk_pin = gpio__construct_with_function(0, 15, GPIO__FUNCTION_2);
  spi_mosi_pin = gpio__construct_with_function(0, 18, GPIO__FUNCTION_2);
  spi_miso_pin = gpio__construct_with_function(0, 17, GPIO__FUNCTION_2);

  // gpio_reset_pin = gpio__construct_with_function(4, 28,
  // GPIO__FUNCITON_0_IO_PIN); gpio_xdcs_pin = gpio__construct_with_function(0,
  // 6, GPIO__FUNCITON_0_IO_PIN); gpio_xcs_pin =
  // gpio__construct_with_function(0, 8, GPIO__FUNCITON_0_IO_PIN); gpio_dreq_pin
  // = gpio__construct_with_function(1, 31, GPIO__FUNCITON_0_IO_PIN);

  gpio_reset_pin = gpio__construct_as_output(4, 28);
  gpio_xdcs_pin = gpio__construct_as_output(0, 6);
  gpio_xcs_pin = gpio__construct_as_output(0, 8);
  gpio_dreq_pin = gpio__construct_as_input(1, 31);

  gpio__set(gpio_xdcs_pin);
  gpio__set(gpio_xcs_pin);
  gpio__set(gpio_reset_pin);
}

void decoder__send_to_sci(uint8_t address, uint8_t high_byte,
                          uint8_t low_byte) {
  decoder__set_xcs();
  while (!decoder__data_ready()) {
    ;
  }

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

  while (!decoder__data_ready()) {
    ;
  }

  ssp0_mp3__send_byte(read_opcode);
  ssp0_mp3__send_byte(address);

  high_byte = ssp0_mp3__send_byte(dummy_code);
  low_byte = ssp0_mp3__send_byte(dummy_code);

  while (!decoder__data_ready()) {
    ;
  }

  decoder__clear_xcs();
  printf("High byte: %x.\n", high_byte);
  printf("Low byte: %x.\n", low_byte);
  uint16_t result = low_byte;
  result |= (high_byte << 5);

  return result;
}

void decoder__send_to_sdi(uint8_t byte_to_transfer) {
  decoder__set_xdcs();

  while (!decoder__data_ready()) {
    ;
  }

  printf("Data ready.\n");

  ssp0_mp3__send_byte(byte_to_transfer);

  while (!decoder__data_ready()) {
    ;
  }

  printf("Send byte.\n");

  decoder__clear_xdcs();
}

// void decoder__play_music(uint8_t data){
//   decoder__send_to_sdi(data);
// }

//----------------------- Pin Functions -------------------------- //

void decoder__set_reset(void) { gpio__reset(gpio_reset_pin); }

void decoder__clear_reset(void) { gpio__set(gpio_reset_pin); }

void decoder__set_xcs(void) { gpio__reset(gpio_xcs_pin); }

void decoder__clear_xcs(void) { gpio__set(gpio_xcs_pin); }

void decoder__set_xdcs(void) { gpio__reset(gpio_xdcs_pin); }

void decoder__clear_xdcs(void) { gpio__set(gpio_xdcs_pin); }

bool decoder__data_ready(void) { return gpio__get(gpio_dreq_pin); }

void decoder__get_status(void) {
  uint16_t mode_reading = decoder__read_from_sci(mode);
  uint16_t status_reading = decoder__read_from_sci(sci_status);
  printf("Mode: %x.\n", mode_reading);
  printf("Status: %x.\n", status_reading);
}
