#include "ssp0_mp3.h"
#include "clock.h"
#include "lpc40xx.h"
#include "lpc_peripherals.h"

const static uint32_t cr0_dss_select_8_bit_transfer = (7 << 0);
const static uint32_t cr1_sse_enable_ssp = (1 << 1);
const static uint32_t sr_bsy_is_busy = (1 << 4);

static void set_control_registers(void);

/*********************************************************************************************************/
//                                          Public Functions //
/*********************************************************************************************************/

void ssp0_mp3__init(void) {
  lpc_peripheral__turn_on_power_to(LPC_PERIPHERAL__SSP0);

  set_control_registers();
}

uint8_t ssp0_mp3__send_byte(uint8_t byte_to_transfer) {
  LPC_SSP0->DR = byte_to_transfer;

  while (LPC_SSP0->SR & sr_bsy_is_busy) {
    ;
  }

  return (uint8_t)LPC_SSP0->DR & 0xFF;
}

void ssp0_mp3__set_prescalar_registers(uint32_t max_clock_mhz) {
  uint8_t divider = 2;
  uint32_t cpu_clock = clock__get_core_clock_hz() / (1000UL * 1000UL);

  while ((max_clock_mhz < (cpu_clock / divider)) && divider <= 254) {
    divider += 2;
  }

  LPC_SSP0->CPSR = divider & 0xFF;
}

/*********************************************************************************************************/
//                                          Private Functions //
/*********************************************************************************************************/

static void set_control_registers(void) {
  LPC_SSP0->CR0 |= cr0_dss_select_8_bit_transfer;
  LPC_SSP0->CR1 |= cr1_sse_enable_ssp;
}
