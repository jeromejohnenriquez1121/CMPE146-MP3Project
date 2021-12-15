#include "mp3_functions.h"
#include "decoder.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

const static uint16_t initial_volume_high_byte = 0x22;
const static uint16_t initial_volume_low_byte = 0x22;

/**************************************************************************/
//                    Variable and Function Declarations
/**************************************************************************/

void mp3_functions__init_volume(void) {
  decoder__send_to_sci(sci_volume_ctl, initial_volume_high_byte,
                       initial_volume_low_byte);
}

void mp3_functions__init_mode(void) { current_mode = menu_mode; }

bool mp3_functions__raise_volume(void) {
  uint16_t this_current_volume = decoder__read_from_sci(sci_volume_ctl);
  uint8_t high_byte = ((this_current_volume >> 8) - 0x22) & 0xFF;
  uint8_t low_byte = ((this_current_volume >> 8) - 0x22) & 0xFF;
  if (high_byte <= 0x22 && low_byte <= 0x22) {
    decoder__send_to_sci(sci_volume_ctl, 0x22, 0x22);
  } else {
    decoder__send_to_sci(sci_volume_ctl, high_byte, low_byte);
  }

  return true;
}

bool mp3_functions__lower_volume(void) {
  uint16_t this_current_volume = decoder__read_from_sci(sci_volume_ctl);
  uint8_t high_byte = ((this_current_volume >> 8) + 0x22) & 0xFF;
  uint8_t low_byte = ((this_current_volume >> 8) + 0x22) & 0xFF;
  if ((high_byte >= 0xDD && low_byte >= 0xDD) ||
      (high_byte <= 0x22 && low_byte <= 0x22)) {
    decoder__send_to_sci(sci_volume_ctl, 0xEE, 0xEE);
  } else {
    decoder__send_to_sci(sci_volume_ctl, high_byte, low_byte);
  }

  return true;
}

int mp3_functions__get_mode(void) { return current_mode; }

char mp3_functions__get_current_volume(void) {
  uint8_t this_current_volume = decoder__read_from_sci(sci_volume_ctl) & 0xF;
  printf("%x\n", this_current_volume);
  char volume = ' ';

  if (this_current_volume == 0x2) {
    volume = '6';

  } else if (this_current_volume == 0x4) {
    volume = '5';

  } else if (this_current_volume == 0x6) {
    volume = '4';

  } else if (this_current_volume == 0x8) {
    volume = '3';

  } else if (this_current_volume == 0xA) {
    volume = '2';

  } else if (this_current_volume == 0xC) {
    volume = '1';

  } else if (this_current_volume == 0xE) {
    volume = '0';
  }

  return volume;
}

void mp3_functions__scroll_through_modes() {
  if (current_mode == menu_mode) {
    current_mode = volume_mode;
  } else if (current_mode == volume_mode) {
    current_mode = bass_mode;
  } else if (current_mode == bass_mode) {
    current_mode = treble_mode;
  } else if (current_mode == treble_mode) {
    current_mode = rewind_skip_mode;
  } else if (current_mode == rewind_skip_mode) {
    current_mode = menu_mode;
  }
}
