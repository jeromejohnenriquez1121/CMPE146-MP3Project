#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum { volume_mode = 0x1, bass_mode = 0x2, treble_mode = 0x3 } mode_e;

typedef enum { increment_vol = 1, decrement_vol = 2 } volume_status_e;

uint16_t current_mode;
uint16_t current_volume;
uint16_t current_bass_level;

uint16_t mp3_functions__get_current_volume(void);

// ------------ Initialize volume and mode ------------ //
void mp3_functions__init_volume(void);
void mp3_functions__init_mode(void);

// ---------------- Change volume ---------------- //
bool mp3_functions__raise_volume(void);
bool mp3_functions__lower_volume(void);

// ---------------- Change bass ---------------- //
void mp3_functions__raise_bass(uint8_t bass_level);
void mp3_functions__lower_bass(uint8_t bass_level);

// ---------------- Change treble ---------------- //
void mp3_functions__raise_treble(uint8_t treble_level);
void mp3_functions__lower_treble(uint8_t treble_level);
