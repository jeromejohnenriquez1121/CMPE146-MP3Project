#include "clock.h"
#include "lpc40xx.h"
#include "lpc_peripherals.h"
#include <stdint.h>

// Max clock is 55.3 or 24? MHz
void ssp2_mp3__init(uint32_t max_clock_hz);

uint8_t ssp2_mp3__transfer_data(uint8_t data_transferred);

//*********************************************************************************************************************

//ssp2_mp3.c

#include "ssp2_mp3.h"

static bool turn_on_peripheral(void)
{
  lpc_peripheral__turn_on_power_to(LPC_PERIPHERAL__SSP2);
  return true;
}

static void set_up_control_registers(uint32_t this_cr0_dss, uint32_t this_cr0_frf, uint32_t this_cr1_sse,
                                     uint32_t this_cr1_ms)
{
  LPC_SSP2->CR0 |= this_cr0_dss;
  LPC_SSP2->CR0 &= this_cr0_frf;
  LPC_SSP2->CR1 |= this_cr1_sse;
  LPC_SSP2->CR1 &= this_cr1_ms;
}

static void set_prescalar_registers(uint32_t this_max_clock_hz)
{
  uint8_t divider = 2;
  uint32_t cpu_clock = clock__get_core_clock_hz() / (1000UL * 1000UL);

  while ((this_max_clock_hz < (cpu_clock / divider)) && (divider <= 254))
  {
    divider += 2;
  }

  LPC_SSP2->CPSR = divider & 0xFF;
}

void ssp2_mp3__init(uint32_t max_clock_hz)
{
  static const uint32_t cr0_dss = (7 << 0);
  static const uint32_t cr0_frf = ~((1 << 5) | (1 << 4));
  static const uint32_t cr1_sse = (1 << 1);
  static const uint32_t cr1_ms = ~(1 << 2);

  set_up_control_registers(cr0_dss, cr0_frf, cr1_sse, cr1_ms);
}

uint8_t ssp2_mp3__transfer_data(uint8_t data_transferred)
{
  LPC_SSP2->DR = data_transferred;
  static const uint32_t sr_bsy = (1 << 4);

  while (LPC_SSP2->SR & sr_bsy)
  {
    ;
  }

  return (uint8_t)LPC_SSP2->DR & 0xFFFF;
}