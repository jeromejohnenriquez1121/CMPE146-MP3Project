#include "mp3_functions.h"
#include "decoder.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

const static uint16_t initial_volume_high_byte = 0x11;
const static uint16_t initial_volume_low_byte = 0x11;

/**************************************************************************/
//                    Variable and Function Declarations
/**************************************************************************/

void mp3_functions__init_volume(void) {
  decoder__send_to_sci(volume_ctl, initial_volume_high_byte,
                       initial_volume_low_byte);
}

void mp3_functions__init_mode(void) { current_mode = volume_mode; }

bool mp3_functions__raise_volume(void) {
  uint16_t this_current_volume = decoder__read_from_sci(volume_ctl);
  uint8_t high_byte = ((this_current_volume >> 8) - 0x11) & 0xFF;
  uint8_t low_byte = ((this_current_volume >> 8) - 0x11) & 0xFF;
  if (high_byte <= 0x11 && low_byte <= 0x11) {
    decoder__send_to_sci(volume_ctl, 0x11, 0x11);
  } else {
    decoder__send_to_sci(volume_ctl, high_byte, low_byte);
  }

  return true;
}

bool mp3_functions__lower_volume(void) {
  uint16_t this_current_volume = decoder__read_from_sci(volume_ctl);
  uint8_t high_byte = ((this_current_volume >> 8) + 0x11) & 0xFF;
  uint8_t low_byte = ((this_current_volume >> 8) + 0x11) & 0xFF;
  if (high_byte >= 0xEE && low_byte >= 0xEE) {
    decoder__send_to_sci(volume_ctl, 0xEE, 0xEE);
  } else {
    decoder__send_to_sci(volume_ctl, high_byte, low_byte);
  }

  return true;
}

uint16_t get_current_volume(void) { return current_volume; }
