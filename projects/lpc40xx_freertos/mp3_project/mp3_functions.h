#pragma once

#include "ff.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum {
  pause_mode = 0x0,
  volume_mode = 0x1,
  bass_mode = 0x2,
  treble_mode = 0x3,
  rewind_skip_mode = 0x4,
} mode_e;

typedef enum { increment_vol = 1, decrement_vol = 2 } volume_status_e;

uint16_t current_mode;
uint16_t current_volume;
uint16_t current_bass_level;

void mp3_functions__scroll_through_modes();

char mp3_functions__get_current_volume(void);

void mp3_functions__enable_pause(bool *pause_var);
void mp3_functions__disable_pause(bool *pause_var);

void mp3_functions__rewind(UINT *br, size_t *song_index, FIL *file);
void mp3_functions__skip(UINT *br, size_t *song_index, FIL *file);

// ------------ Initialize volume and mode ------------ //
void mp3_functions__init_volume(void);
void mp3_functions__init_mode(void);

// ---------------- Change volume ---------------- //
bool mp3_functions__raise_volume(void);
bool mp3_functions__lower_volume(void);

// ---------------- Change bass ---------------- //
void mp3_functions__raise_bass(void);
void mp3_functions__lower_bass(void);

// ---------------- Change treble ---------------- //
void mp3_functions__raise_treble(void);
void mp3_functions__lower_treble(void);
