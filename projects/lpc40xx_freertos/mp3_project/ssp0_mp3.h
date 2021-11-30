#include "gpio.h"
#include "lpc40xx.h"

#pragma once

void ssp0_mp3__init(void);

uint8_t ssp0_mp3__send_byte(uint8_t byte_to_transfer);

void ssp0_mp3__set_prescalar_registers(uint32_t max_clock_mhz);
