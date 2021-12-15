#include "mp3_functions.h"
#include "FreeRTOS.h"
#include "decoder.h"
#include "ff.h"
#include "queue.h"
#include "song_list.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

QueueHandle_t song_name_queue;

const static uint16_t initial_volume_high_byte = 0x22;
const static uint16_t initial_volume_low_byte = 0x22;

/**************************************************************************/
//                          Public Functions
/**************************************************************************/

// ------------ Volume Functions ---------- //

void mp3_functions__init_volume(void) {
  decoder__send_to_sci(sci_volume_ctl, initial_volume_high_byte,
                       initial_volume_low_byte);
}

void mp3_functions__init_mode(void) { current_mode = pause_mode; }

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

// ------------ Mode Functions ---------- //

void mp3_functions__scroll_through_modes() {
  if (current_mode == pause_mode) {
    current_mode = volume_mode;
  } else if (current_mode == volume_mode) {
    current_mode = bass_mode;
  } else if (current_mode == bass_mode) {
    current_mode = treble_mode;
  } else if (current_mode == treble_mode) {
    current_mode = rewind_skip_mode;
  } else if (current_mode == rewind_skip_mode) {
    current_mode = pause_mode;
  }
}

//---------------- Play and Pauses Functions ----------------------- //

void mp3_functions__enable_pause(bool *pause_var) { *pause_var = true; }

void mp3_functions__disable_pause(bool *pause_var) { *pause_var = false; }

//-------------- Adjust Bass and Treble Functions ---------- //

void mp3_functions__raise_bass(void) {
  uint8_t this_current_bass_level =
      (decoder__read_from_sci(sci_bass) >> 4) & 0xF;
  uint8_t new_bass_level = 0x0;

  if (this_current_bass_level > 0xE || this_current_bass_level < 0x2) {
    new_bass_level = 0xF;
  } else {
    new_bass_level -= (this_current_bass_level + 0x3) << 4;
  }

  decoder__send_to_sci(sci_bass, 0x0, new_bass_level);
}
void mp3_functions__lower_bass(void) {
  uint8_t this_current_bass_level =
      (decoder__read_from_sci(sci_bass) >> 4) & 0xF;
  uint8_t new_bass_level = 0x0;

  if (this_current_bass_level < 0 || this_current_bass_level > 0xD) {
    new_bass_level = 0x0;
  } else {
    new_bass_level -= (this_current_bass_level - 0x3) << 4;
  }

  decoder__send_to_sci(sci_bass, 0x0, new_bass_level);
}

void mp3_functions__raise_treble(void) {
  uint8_t this_current_treble_level =
      (decoder__read_from_sci(sci_bass >> 12) & 0xF);
  uint8_t this_new_treble_level = 0x0;

  if (this_current_treble_level > 0xE || this_current_treble_level < 0x2) {
    this_new_treble_level = 0xF;
  } else {
    this_new_treble_level = (this_current_treble_level + 0x3) << 12;
  }
}
void mp3_functions__lower_treble(void) {
  uint8_t this_current_treble_level =
      (decoder__read_from_sci(sci_bass >> 12) & 0xF);
  uint8_t this_new_treble_level = 0x0;

  if (this_current_treble_level > 0xE || this_current_treble_level < 0x2) {
    this_new_treble_level = 0xF;
  } else {
    this_new_treble_level = (this_current_treble_level + 0x3) << 12;
  }
}