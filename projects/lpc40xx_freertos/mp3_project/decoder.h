#include "gpio.h"
#include <stdint.h>

#pragma once

// --------------- Read / Write Opcodes ------------- //
typedef enum { write_opcode = 0x2, read_opcode = 0x3 } opcode_e;

// -------------------- SJTwo Board Pins ----------------- //
gpio_s spi_sclk_pin;
gpio_s spi_mosi_pin;
gpio_s spi_miso_pin;
gpio_s gpio_reset_pin;
gpio_s gpio_xdcs_pin;
gpio_s gpio_xcs_pin;
gpio_s gpio_dreq_pin;

gpio_s gpio_up_button;
gpio_s gpio_down_button;
gpio_s gpio_pause_button;
// ------------------- VS1053 Register Addresses ------------------- //
typedef enum {
  mode = 0x0,
  sci_status = 0x1,
  bass = 0x2,
  clock_freq = 0x3,
  decode_time = 0x4,
  audio_data = 0x5,
  wram = 0x6,
  wram_addr = 0x7,
  hdat0 = 0x8,
  hdat1 = 0x9,
  aiaddr = 0xA,
  volume_ctl = 0xB,
  aictrl0 = 0xC,
  aictrl1 = 0xD,
  aictrl2 = 0xE,
  aictrl3 = 0xF
} sci_registers_e;

// -------------- Decoder Functions ------------- //
void decoder__initialize(uint32_t max_clock_as_mhz);

void decoder__set_pins(void);

void decoder__get_status(void);

void decoder__send_to_sci(uint8_t address, uint8_t high_byte, uint8_t low_byte);

uint16_t decoder__read_from_sci(uint8_t address);

void decoder__send_to_sdi(uint8_t byte_to_transfer);

void decoder__set_reset(void);

void decoder__clear_reset(void);

void decoder__pause(void);

// ------------- Decoder Chip Selects ------------ //
void decoder__set_xcs(void);
void decoder__clear_xcs(void);

void decoder__set_xdcs(void);
void decoder__clear_xdcs(void);

// -------------- DREQ Reading Function -------------- //
bool decoder__data_ready(void);

// -------------- Volume Functions -------------- //
void decoder__raise_volume(void);
void decoder__lower_volume(void);
