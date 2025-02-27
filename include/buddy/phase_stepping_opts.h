#pragma once
#include <option/has_burst_stepping.h>

namespace phase_stepping::opts {

static constexpr int MOTOR_PERIOD = 1024; // The number of ticks per electrical period of the Trinamic driver
static constexpr int SIN_FRACTION = 4;
static constexpr int SIN_PERIOD = SIN_FRACTION * MOTOR_PERIOD;
static constexpr int SIN_LUT_FRACTIONAL = 15; // The number of bits used for the fractional part of the sine LUT
static constexpr int MAG_FRACTIONAL = 8; // The number of bits used for the fractional part of the magnitude
static constexpr int CURRENT_AMPLITUDE = 248;
static constexpr int CORRECTION_HARMONICS = 16;
static constexpr int SUPPORTED_AXIS_COUNT = 2;
static constexpr int SERIAL_DECIMALS = 5; // Float decimals written in the serial output
static constexpr int ALLOWED_MISSED_TX = 5000;
static constexpr int REFRESH_FREQ = option::has_burst_stepping
    ? 10000
    : 40000;
static constexpr int TICK_FREQ = 1'000'000;
static constexpr int REFRESH_PERIOD_US = TICK_FREQ / REFRESH_FREQ;
static constexpr int GPIO_BUFFER_SIZE = 200;

} // namespace phase_stepping::opts

#define PHSTEP_TIMER_ISR_HANDLER TIM8_UP_TIM13_IRQHandler

// You might be wondering why we use hard-coded constants and we don't read
// them from handles: it is wasted 500 ns - that is 2 % of CPU load
#define PHSTEP_TMC_SPI             SPI3
#define PHSTEP_TMC_DMA             DMA1_Stream5
#define PHSTEP_TMC_DMA_REGS        reinterpret_cast<DMA_Base_Registers *>((((uint32_t)PHSTEP_TMC_DMA & (uint32_t)(~0x3FFU)) + 4U))
#define PHSTEP_TMC_DMA_REGS_OFFSET 6U

#define BURST_DMA             DMA2_Stream1
#define BURST_DMA_REGS        reinterpret_cast<DMA_Base_Registers *>((((uint32_t)BURST_DMA & (uint32_t)(~0x3FFU))))
#define BURST_DMA_REGS_OFFSET 6U
