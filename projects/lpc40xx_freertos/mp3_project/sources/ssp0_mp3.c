#include "ssp0_mp3.h"
#include "clock.h"
#include "lpc40xx.h"
#include "lpc_peripherals.h"
#include <stdbool.h>

// SPI bitmasks for binary options. Each bit mask variable has the format:
// register_symbol_description = operation
const static uint32_t cr0_dss_select_8_bit_transfer = (7 << 0);
const static uint32_t cr0_dss_select_16_bit_transfer = (15 << 0);
const static uint32_t cr1_sse_enable_ssp = (1 << 1);
const static uint32_t sr_bsy_is_busy = (1 << 4);

// Static functions
static void set_control_registers();
static void set_prescalar_registers(uint32_t max_clock_mhz);

/*********************************************************************************************************/
//                                            Public Functions
/*********************************************************************************************************/

void ssp0_mp3__init(uint32_t max_clock_mhz) {
  lpc_peripheral__turn_on_power_to(LPC_PERIPHERAL__SSP0);

  set_control_registers();

  set_prescalar_registers(max_clock_mhz);
}

uint8_t ssp0_mp3__send_byte(uint8_t byte_to_transfer) {
  LPC_SSP0->DR = byte_to_transfer;

  while (LPC_SSP0->SR & sr_bsy_is_busy) {
    ;
  }
  return (uint8_t)LPC_SSP0->DR;
}

/*********************************************************************************************************/
//                                          Private Functions
/*********************************************************************************************************/

static void set_control_registers(void) {
  LPC_SSP0->CR0 |= cr0_dss_select_8_bit_transfer;
  LPC_SSP0->CR1 |= cr1_sse_enable_ssp;
}

static void set_prescalar_registers(uint32_t max_clock_mhz) {
  uint8_t divider = 2;
  uint32_t cpu_clock_mhz = clock__get_core_clock_hz() / (1000UL * 1000UL);

  while ((max_clock_mhz < (cpu_clock_mhz / divider)) && divider <= 254) {
    divider += 2;
  }

  LPC_SSP0->CPSR = divider & 0xFF;
}