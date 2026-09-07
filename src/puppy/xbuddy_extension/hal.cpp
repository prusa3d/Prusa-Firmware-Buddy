/// @file
#include "hal.hpp"

#include "option/extension_variant.h"
#include "hal_clock.hpp"
#include "hal_ext_fs.hpp"
#include "hal_mmu_port.hpp"
#include "hal_pub.hpp"
#include "hal_rs485.hpp"
#include "hal_rng.hpp"
#include "hal_usb.hpp"
#include <array>
#include <bitset>
#include <freertos/timing.hpp>
#include <stm32h5xx_hal.h>
#include <stm32h5xx_ll_gpio.h>

#include <utils/timing/timer_event_period_tracker.hpp>

#include <option/has_mmu2.h>
#if HAS_MMU2()
    #include "hal_mmu.hpp"
#endif

#if HAS_GPIO_EXPANDER()
    #include "hal_gpio_expander.hpp"
#endif

// Default prescaler for our timers.
// 6 MHz clock (30 MHz peripheral clock, *2 to timer, /10 prescaler)
static constexpr uint32_t default_prescaler = 10;

extern "C" void HAL_UART_MspInit(UART_HandleTypeDef *huart) {
    hal::rs485::msp_init(huart);
#if HAS_MMU2()
    hal::mmu::msp_init(huart);
#endif
}

extern "C" void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
    hal::rs485::error_callback(huart);
}

extern "C" void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size) {
    hal::rs485::rx_callback(huart, size);
#if HAS_MMU2()
    hal::mmu::rx_callback(huart, size);
#endif
}

extern "C" void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
    hal::rs485::tx_callback(huart);
#if HAS_MMU2()
    hal::mmu::tx_callback(huart);
#endif
}

extern "C" void HAL_ADC_MspInit(ADC_HandleTypeDef *hadc) {
    GPIO_InitTypeDef GPIO_InitStruct = {};
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {};
    if (hadc->Instance == ADC1) {
        /** Initializes the peripherals clock
         */
        PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_ADCDAC;
        PeriphClkInitStruct.AdcDacClockSelection = RCC_ADCDACCLKSOURCE_HCLK;
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK) {
            abort();
        }

        /* Peripheral clock enable */
        __HAL_RCC_ADC_CLK_ENABLE();

        __HAL_RCC_GPIOB_CLK_ENABLE();
        /**ADC1 GPIO Configuration
        PB1     ------> ADC1_INP5
        */
        GPIO_InitStruct.Pin = GPIO_PIN_1;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    }
}

static void tim1_postinit() {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**TIM1 GPIO Configuration
    PA8     ------> TIM1_CH1
    PA9     ------> TIM1_CH2
    PA10    ------> TIM1_CH3
    */
    constexpr GPIO_InitTypeDef GPIO_InitStruct {
#if EXTENSION_IS_IX()
        // iX has the filament sensor on PA9
        .Pin = GPIO_PIN_8 | GPIO_PIN_10,
#else
        .Pin = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10,
#endif
        .Mode = GPIO_MODE_AF_PP,
        .Pull = GPIO_NOPULL,
        .Speed = GPIO_SPEED_FREQ_LOW,
        .Alternate = GPIO_AF1_TIM1,
    };
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    HAL_NVIC_SetPriority(TIM1_CC_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(TIM1_CC_IRQn);
    HAL_NVIC_SetPriority(TIM1_UP_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(TIM1_UP_IRQn);
}

static void tim2_postinit() {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**TIM2 GPIO Configuration
    PA0     ------> TIM2_CH1
    PA1     ------> TIM2_CH2
    PA2     ------> TIM2_CH3
    PA3     ------> TIM2_CH4
    */
    constexpr GPIO_InitTypeDef GPIO_InitStruct {
        .Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3,
        .Mode = GPIO_MODE_AF_PP,
        .Pull = GPIO_NOPULL,
        .Speed = GPIO_SPEED_FREQ_LOW,
        .Alternate = GPIO_AF1_TIM2,
    };
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

static void tim3_postinit() {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**TIM3 GPIO Configuration
    PA6     ------> TIM3_CH1
    PA7     ------> TIM3_CH2
    PB0     ------> TIM3_CH3
    */
    GPIO_InitTypeDef GPIO_InitStruct {
        .Pin = GPIO_PIN_7
            | (PA6_PIN_DRIVES_W_LED() ? GPIO_PIN_6 : 0) //
        ,
        .Mode = GPIO_MODE_AF_PP,
        .Pull = GPIO_NOPULL,
        .Speed = GPIO_SPEED_FREQ_LOW,
        .Alternate = GPIO_AF2_TIM3,
    };
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

static void tim1_init() {
    // reset peripheral
    __HAL_RCC_TIM1_CLK_ENABLE();
    __HAL_RCC_TIM1_FORCE_RESET();
    __HAL_RCC_TIM1_RELEASE_RESET();

    // input mode, without remapping
    constexpr const uint32_t capture_compare_selection = 0b01;

    // 0110:fSAMPLING = fDTS/4, N = 6; was getting false edges on fans with longer wire - BFW-7090
    constexpr const uint32_t input_capture_filter = 0b0111;

    // no prescaler, capture is done each time an edge is detected on the capture input
    constexpr const uint32_t input_capture_prescaler = 0b00;

    // configure channel 1 & 2
    TIM1->CCMR1 = 0
        | (capture_compare_selection << TIM_CCMR1_CC1S_Pos)
        | (input_capture_prescaler << TIM_CCMR1_IC1PSC_Pos)
        | (input_capture_filter << TIM_CCMR1_IC1F_Pos)
        | (capture_compare_selection << TIM_CCMR1_CC2S_Pos)
        | (input_capture_prescaler << TIM_CCMR1_IC2PSC_Pos)
        | (input_capture_filter << TIM_CCMR1_IC2F_Pos);

    // configure channel 3 & 4
    TIM1->CCMR2 = 0
        | (capture_compare_selection << TIM_CCMR2_CC3S_Pos)
        | (input_capture_prescaler << TIM_CCMR2_IC3PSC_Pos)
        | (input_capture_filter << TIM_CCMR2_IC3F_Pos)
        | (capture_compare_selection << TIM_CCMR2_CC4S_Pos)
        | (input_capture_prescaler << TIM_CCMR2_IC4PSC_Pos)
        | (input_capture_filter << TIM_CCMR2_IC4F_Pos);

    // enable input of channels 1 & 2 & 3
    TIM1->CCER = TIM_CCER_CC1E | TIM_CCER_CC2E | TIM_CCER_CC3E;

    // enable interrupts on channels
    TIM1->DIER = TIM_DIER_UIE | TIM_DIER_CC1IE | TIM_DIER_CC2IE | TIM_DIER_CC3IE;

    // 1 MHz clock (30 MHz peripheral clock, *2 to timer, /60 prescaler)
    // This gives us 16-bit counter overflow every 65ms.
    // Period is about 2ms at max RPM and 32ms at min RPM so we should be good.
    TIM1->PSC = 60 - 1;

    // enable counter
    TIM1->CR1 = TIM_CR1_CEN | TIM_CR1_URS;
}

static void tim2_init() {
    // reset peripheral
    __HAL_RCC_TIM2_CLK_ENABLE();
    __HAL_RCC_TIM2_FORCE_RESET();
    __HAL_RCC_TIM2_RELEASE_RESET();

    // channel 1 settings
    TIM2->CCMR1 |= (TIM_OCMODE_PWM1 << 0) | TIM_CCMR1_OC1PE;

    // channel 2 settings
    TIM2->CCMR1 |= (TIM_OCMODE_PWM1 << 8) | TIM_CCMR1_OC2PE;

    // channel 3 settings
    TIM2->CCMR2 |= (TIM_OCMODE_PWM1 << 0) | TIM_CCMR2_OC3PE;

    // channel 4 settings
    TIM2->CCMR2 |= (TIM_OCMODE_PWM1 << 8) | TIM_CCMR2_OC4PE;

    // enable output of channels
    TIM2->CCER = TIM_CCER_CC1E | TIM_CCER_CC2E | TIM_CCER_CC3E | TIM_CCER_CC4E;

    // 6 MHz clock (30 MHz peripheral clock, *2 to timer, /10 prescaler)
    TIM2->PSC = default_prescaler - 1;

    // auto-reload value
    // 6 MHz / 255 gives ~25 kHz for PWM which is super good enough.
    // It also simplifies set_pwm() functions a lot.
    TIM2->ARR = 255;

    // enable counter
    TIM2->CR1 |= TIM_CR1_CEN;
}

static void tim3_init() {
    // reset peripheral
    __HAL_RCC_TIM3_CLK_ENABLE();
    __HAL_RCC_TIM3_FORCE_RESET();
    __HAL_RCC_TIM3_RELEASE_RESET();

#if PA6_PIN_DRIVES_W_LED()
    // channel 1 settings
    TIM3->CCMR1 |= (TIM_OCMODE_PWM1 << 0) | TIM_CCMR1_OC1PE;
#endif

    // channel 2 settings
    TIM3->CCMR1 |= (TIM_OCMODE_PWM2 << 8) | TIM_CCMR1_OC2PE;

    // channel 3 settings
    TIM3->CCMR2 |= (TIM_OCMODE_PWM2 << 0) | TIM_CCMR2_OC3PE;

    // enable output of channels
    TIM3->CCER = TIM_CCER_CC2E | TIM_CCER_CC3E
        | (PA6_PIN_DRIVES_W_LED() ? TIM_CCER_CC1E : 0) //
        ;

    // 6 MHz clock (30 MHz peripheral clock, *2 to timer, /10 prescaler)
    TIM3->PSC = default_prescaler - 1;

    // auto-reload value
    // 6 MHz / 255 gives ~25 kHz for PWM which is super good enough.
    // It also simplifies set_pwm() functions a lot.
    TIM3->ARR = 255;

    // enable counter
    TIM3->CR1 |= TIM_CR1_CEN;
}

ADC_HandleTypeDef hadc1;

static void MX_ADC1_Init(void) {
    hadc1.Instance = ADC1;
    hadc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV4;
    hadc1.Init.Resolution = ADC_RESOLUTION_12B;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
    hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    hadc1.Init.LowPowerAutoWait = DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.NbrOfConversion = 1;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.DMAContinuousRequests = DISABLE;
    hadc1.Init.SamplingMode = ADC_SAMPLING_MODE_NORMAL;
    hadc1.Init.Overrun = ADC_OVR_DATA_PRESERVED;
    hadc1.Init.OversamplingMode = DISABLE;
    if (HAL_ADC_Init(&hadc1) != HAL_OK) {
        abort();
    }

    static constexpr auto single_diff = ADC_SINGLE_ENDED;

    ADC_ChannelConfTypeDef sConfig = {};
    sConfig.Channel = ADC_CHANNEL_5;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_2CYCLES_5;
    sConfig.SingleDiff = single_diff;
    sConfig.OffsetNumber = ADC_OFFSET_NONE;
    sConfig.Offset = 0;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
        abort();
    }

    HAL_ADCEx_Calibration_Start(&hadc1, single_diff);
}
// Tracking TIM1 increments between two edges on the input pins (fan tacho)
TimerEventPeriodTracker tim1_cc1;
TimerEventPeriodTracker tim1_cc2;
TimerEventPeriodTracker tim1_cc3;

// Shared IRQ handler for TIM1 capture and overflow
void TIM1_IRQHandler() {
    uint32_t events = 0;
    uint32_t CCR1 = 0;
    uint32_t CCR2 = 0;
    uint32_t CCR3 = 0;

    // Rinse and repeat if there were new events during data capture
    // If overflow happened during reading out the CCR registers, we need to report it correctly as a simultaneous event to the PeriodTracker
    while (true) {
        const auto prev_events = events;
        events |= TIM1->SR & (TIM_SR_CC1IF | TIM_SR_CC2IF | TIM_SR_CC3IF | TIM_SR_UIF);
        if (events == prev_events) {
            break;
        }

        // Read out the CCR registers AFTER we've read whether there was an capture (so after we are sure that they contain valid data)
        // Reading out CCRx registers clears the CCxIF flags, so we need to only read them if we are going to handle them to prevent losing the IF flags
        if (events & TIM_SR_CC1IF) {
            CCR1 = TIM1->CCR1;
        }

        if (events & TIM_SR_CC2IF) {
            CCR2 = TIM1->CCR2;
        }

        if (events & TIM_SR_CC3IF) {
            CCR3 = TIM1->CCR3;
        }
    }

    // Clear out the flags in the SR that we know we will be handling
    // The SR registers are rc_w0, meaning writing 1 has no effect and writing 0 clears the register
    // This is the right and safe way to only clear the flags that we want
    TIM1->SR = ~events;

    // TODO: we could possibly implement CCxOF detection that would indicate that we have missed processing a capture (edge)

    const bool was_overflow = (events & TIM_SR_UIF);
    tim1_cc1.handle_multi_event(CCR1, events & TIM_SR_CC1IF, was_overflow);
    tim1_cc2.handle_multi_event(CCR2, events & TIM_SR_CC2IF, was_overflow);
    tim1_cc3.handle_multi_event(CCR3, events & TIM_SR_CC3IF, was_overflow);
}

extern "C" void TIM1_UP_IRQHandler() {
    TIM1_IRQHandler();
}

extern "C" void TIM1_CC_IRQHandler() {
    TIM1_IRQHandler();
}

static constexpr auto FSENSOR_PIN = EXTENSION_IS_IX() ? GPIO_PIN_9 : GPIO_PIN_5;

static void filament_sensor_pins_init() {
    constexpr GPIO_InitTypeDef GPIO_InitStruct {
        .Pin = FSENSOR_PIN,
        .Mode = GPIO_MODE_INPUT,
        .Pull = EXTENSION_IS_IX() ? GPIO_PULLUP : GPIO_PULLDOWN,
        .Speed = GPIO_SPEED_FREQ_LOW,
        .Alternate = 0,
    };
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

void hal::init() {
    HAL_Init();
    HAL_ICACHE_Enable();
    hal::overclock();
    tim1_init();
    tim2_init();
    tim3_init();
    tim1_postinit();
    tim2_postinit();
    tim3_postinit();
    MX_ADC1_Init();
    hal::rs485::init();
    hal::mmu_port::init();
#if HAS_MMU2()
    hal::mmu::init();
#endif
#if EXTENSION_IS_XL_CAN()
    hal::fan_power::init();
#endif
#if HAS_GPIO_EXPANDER()
    hal::gpio_expander::init();
#endif

    hal::usb::init();
#if EXTENSION_IS_IX()
    hal::usb::power_pin_set(true);
#else
    hal::usb::power_pin_set(false);
#endif

    hal::pub::init();
    filament_sensor_pins_init();
    hal::rng::init();
    hal::ext_fs::init();
}

void hal::setup() {
    hal::ext_fs::setup();
}

void hal::panic() {
    asm volatile("bkpt 0");
    for (;;)
        ;
}

extern "C" void hal_panic() {
    hal::panic();
}

#define ISR_HANDLER(name)    \
    extern "C" void name() { \
        hal::panic();        \
    }
ISR_HANDLER(NMI_Handler)
ISR_HANDLER(HardFault_Handler)
ISR_HANDLER(MemManage_Handler)
ISR_HANDLER(BusFault_Handler)
ISR_HANDLER(UsageFault_Handler)
ISR_HANDLER(DebugMon_Handler)
#undef ISR_HANDLER

// SVC_Handler + PendSV_Handler + SysTick_Handler are defined by FreeRTOS
// Note that this means HAL_Delay() doesn't work because nobody is calling
// HAL_IncTick() but that is OK since we should not be using HAL functions
// which perform busy-waiting anyway.

static uint32_t temperature_raw = 0;

/// Single GPIO filament sensor (PA5 on standard, PA9 on iX)
static hal::filament_sensor::State gpio_fs_state = hal::filament_sensor::State::uninitialized;
static size_t gpio_fs_last_millis = 0;

#if !EXTENSION_IS_IX()
/// PA5 debounce: 4 phases with alternating pull-up/pull-down (unused on iX)
static std::bitset<4> gpio_fs_raw;
static uint8_t gpio_fs_phase = 0;
#endif

static void step_temperature_adc() {
    // Until we have a non-blocking DMA or interrupt based ADC, we do this
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
    temperature_raw = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);
}

static void step_filament_sensor() {
    const auto now = freertos::millis();

    // --- Single GPIO sensor (FSENSOR_PIN on GPIOA) ---
    if (now - gpio_fs_last_millis > 10) {
        gpio_fs_last_millis = now;

#if EXTENSION_IS_IX()
        gpio_fs_state = (HAL_GPIO_ReadPin(GPIOA, FSENSOR_PIN) == GPIO_PIN_SET)
            ? hal::filament_sensor::State::no_filament
            : hal::filament_sensor::State::has_filament;
#else
        gpio_fs_raw[gpio_fs_phase] = (HAL_GPIO_ReadPin(GPIOA, FSENSOR_PIN) == GPIO_PIN_SET);
        gpio_fs_phase = (gpio_fs_phase + 1) % 4;

        // Set up the pull for the next phase, use the time between phases to stabilize the readout
        LL_GPIO_SetPinPull(GPIOA, GPIO_PIN_5, gpio_fs_phase % 2 ? GPIO_PULLUP : GPIO_PULLDOWN);

        switch (gpio_fs_raw.to_ulong()) {
        case 0b1111:
            gpio_fs_state = hal::filament_sensor::State::has_filament;
            break;

        case 0b0000:
            gpio_fs_state = hal::filament_sensor::State::no_filament;
            break;

        case 0b1010:
            // The readout followed exactly the pullup changes -> there's nothing connected
            gpio_fs_state = hal::filament_sensor::State::disconnected;
            break;

        default:
            // The filament could have been inserted/removed between the phases, wait for definitive values
            break;
        }
#endif
    }
}

void hal::step() {
    step_temperature_adc();
    step_filament_sensor();
    hal::ext_fs::step();
}

static uint32_t tim1_period_to_rpm(const TimerEventPeriodTracker &tracker) {
    // Disable TIM1 interrupts while reading the period to avoid race conditions
    const auto prev_dier = TIM1->DIER;
    TIM1->DIER = 0;
    const auto period = tracker.period_unsafe();
    TIM1->DIER = prev_dier;

    if (period == TimerEventPeriodTracker::invalid_period || period == 0) {
        return 0;
    }
    // 60 seconds in minute, 1 MHZ timer, 2 rising edges per revolution
    return (60 * 1'000'000 / 2) / period;
}

// fan1 and fan2 are connected to the same pwm signal
static void fan1_fan2_set_pwm(hal::DutyCycle duty_cycle) {
    TIM3->CCR2 = duty_cycle;
}

void hal::fan1::set_pwm(DutyCycle duty_cycle) {
    fan1_fan2_set_pwm(duty_cycle);
}

uint32_t hal::fan1::get_rpm() {
    return tim1_period_to_rpm(tim1_cc1);
}

void hal::fan2::set_pwm(DutyCycle duty_cycle) {
    fan1_fan2_set_pwm(duty_cycle);
}

uint32_t hal::fan2::get_rpm() {
    // When debugging with just one fan, it might be useful to uncomment
    // following line because pwm is shared and motherboard goes crazy
    // when only one of the fans is spinning...
    // return fan1::get_rpm();
    return tim1_period_to_rpm(tim1_cc2);
}

void hal::fan3::set_pwm(DutyCycle duty_cycle) {
    TIM3->CCR3 = duty_cycle;
}

uint32_t hal::fan3::get_rpm() {
    return tim1_period_to_rpm(tim1_cc3);
}

#if EXTENSION_IS_XL_CAN()

void hal::fan_power::init() {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    // Park the gate off before switching PA6 to output: the board pull-up
    // holds the active-low EN high (off) from MCU reset until here, and
    // pre-loading ODR makes the mode switch glitch-free.
    enable_pin_set(false);
    constexpr GPIO_InitTypeDef enable {
        .Pin = GPIO_PIN_6,
        .Mode = GPIO_MODE_OUTPUT_PP,
        .Pull = GPIO_NOPULL,
        .Speed = GPIO_SPEED_FREQ_LOW,
        .Alternate = 0,
    };
    static_assert(PA6_PIN_DRIVES_FAN_POWER());
    HAL_GPIO_Init(GPIOA, &enable);

    // Open-drain fault from the TPS2041C, board 10k pull-up.
    constexpr GPIO_InitTypeDef fault {
        .Pin = GPIO_PIN_13,
        .Mode = GPIO_MODE_INPUT,
        .Pull = GPIO_NOPULL,
        .Speed = GPIO_SPEED_FREQ_LOW,
        .Alternate = 0,
    };
    static_assert(!HAS_GPIO_EXPANDER(), "PB13 is used by the GPIO expander");
    HAL_GPIO_Init(GPIOB, &fault);
}

void hal::fan_power::enable_pin_set(bool enabled) {
    static_assert(PA6_PIN_DRIVES_FAN_POWER());
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, enabled ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

bool hal::fan_power::fault_pin_get() {
    static_assert(!HAS_GPIO_EXPANDER(), "PB13 is used by the GPIO expander");
    return HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_13) == GPIO_PIN_RESET;
}

#endif

#if PA6_PIN_DRIVES_W_LED()
void hal::w_led::set_pwm(DutyCycle duty_cycle) {
    TIM3->CCR1 = duty_cycle;
}

void hal::w_led::set_frequency(uint16_t freq) {
    uint32_t prescaler = default_prescaler;
    if (freq != 0) {
        // Non-default feq requested.
        const uint32_t timer_base_freq = 60000000;
        const uint32_t timer_full_cycle = timer_base_freq / 255; // Counts down from this every full cycle
        prescaler = timer_full_cycle / freq;
    }

    TIM3->PSC = prescaler - 1;
}
#endif

void hal::rgbw_led::set_r_pwm(DutyCycle duty_cycle) {
    TIM2->CCR4 = duty_cycle;
}

void hal::rgbw_led::set_g_pwm(DutyCycle duty_cycle) {
    TIM2->CCR3 = duty_cycle;
}

void hal::rgbw_led::set_b_pwm(DutyCycle duty_cycle) {
    TIM2->CCR2 = duty_cycle;
}

void hal::rgbw_led::set_w_pwm(DutyCycle duty_cycle) {
    TIM2->CCR1 = duty_cycle;
}

uint32_t hal::temperature::get_raw() {
    return temperature_raw;
}

hal::filament_sensor::State hal::filament_sensor::get_gpio() {
    return gpio_fs_state;
}
