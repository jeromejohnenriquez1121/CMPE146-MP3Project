#include <stdint.h>
#pragma once

void ssp0_mp3__init(uint32_t max_clock_mhz);

uint8_t ssp0_mp3__send_byte(uint8_t byte_to_transfer);
