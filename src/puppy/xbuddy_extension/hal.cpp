///@file
#include "hal.hpp"

#include "hal_clock.hpp"
#include <stm32h5xx_hal.h>
#include <freertos/binary_semaphore.hpp>
#include <freertos/stream_buffer.hpp>

static UART_HandleTypeDef huart_rs485;
static std::byte rx_buf_rs485[256];
static volatile size_t rx_len_rs485;
static freertos::BinarySemaphore tx_semaphore_rs485;

static UART_HandleTypeDef huart_mmu;
static std::byte rx_byte_mmu;
static std::span<const std::byte> rx_byte_span_mmu { &rx_byte_mmu, 1 };
static freertos::StreamBuffer<32> rx_mmu_buffer;
static freertos::BinarySemaphore tx_semaphore_mmu;

extern "C" void USART3_IRQHandler(void) {
    HAL_UART_IRQHandler(&huart_rs485);
}

extern "C" void USART2_IRQHandler(void) {
    HAL_UART_IRQHandler(&huart_mmu);
}

extern "C" void HAL_UART_MspInit(UART_HandleTypeDef *huart) {
    GPIO_InitTypeDef GPIO_InitStruct {};
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct {};
    if (huart->Instance == USART3) {
        PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USART3;
        PeriphClkInitStruct.Usart3ClockSelection = RCC_USART3CLKSOURCE_PCLK1;
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK) {
            abort();
        }
        __HAL_RCC_USART3_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();
        GPIO_InitStruct.Pin = GPIO_PIN_7 | GPIO_PIN_8;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF13_USART3;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
        GPIO_InitStruct.Pin = GPIO_PIN_14;
        GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
        HAL_NVIC_SetPriority(USART3_IRQn, 5, 0);
        HAL_NVIC_EnableIRQ(USART3_IRQn);
    }
    if (huart->Instance == USART2) {
        PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USART2;
        PeriphClkInitStruct.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK) {
            abort();
        }
        __HAL_RCC_USART2_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();
        GPIO_InitStruct.Pin = GPIO_PIN_15;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF9_USART2;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
        __HAL_RCC_GPIOB_CLK_ENABLE();
        GPIO_InitStruct.Pin = GPIO_PIN_4;
        GPIO_InitStruct.Alternate = GPIO_AF13_USART2;

        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
        HAL_NVIC_SetPriority(USART2_IRQn, 5, 0);
        HAL_NVIC_EnableIRQ(USART2_IRQn);
    }
}

extern "C" void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
    // TODO
    (void)huart;
}

static void rx_callback_rs485(UART_HandleTypeDef *huart, uint16_t size) {
    if (size == 0) {
        HAL_UARTEx_ReceiveToIdle_IT(huart, (uint8_t *)rx_buf_rs485, sizeof(rx_buf_rs485));
    } else {
        rx_len_rs485 = size;
        long task_woken = tx_semaphore_rs485.release_from_isr();
        // TODO We could wake up correct task here, but there is no freertos:: wrapper at the moment
        (void)task_woken;
    }
}

static void rx_callback_mmu(UART_HandleTypeDef *huart, uint16_t size) {
    if (size) {
        // If the buffer is full, we just start dropping bytes and that's ok.
        (void)rx_mmu_buffer.send_from_isr(rx_byte_span_mmu);
    }
    HAL_UARTEx_ReceiveToIdle_IT(huart, (uint8_t *)&rx_byte_mmu, 1);
}

extern "C" void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size) {
    if (huart == &huart_rs485) {
        rx_callback_rs485(huart, size);
    } else if (huart == &huart_mmu) {
        rx_callback_mmu(huart, size);
    }
}

static void tx_callback_rs485(UART_HandleTypeDef *huart) {
    HAL_UARTEx_ReceiveToIdle_IT(huart, (uint8_t *)rx_buf_rs485, sizeof(rx_buf_rs485));
}

static void tx_callback_mmu(UART_HandleTypeDef *huart) {
    (void)huart;
    // TODO We could wake up correct task here, but there is no freertos:: wrapper at the moment
    (void)tx_semaphore_mmu.release_from_isr();
}

extern "C" void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart == &huart_rs485) {
        tx_callback_rs485(huart);
    } else if (huart == &huart_mmu) {
        tx_callback_mmu(huart);
    }
}

void rs485_init() {
    huart_rs485.Instance = USART3;
    huart_rs485.Init.BaudRate = 230'400;
    huart_rs485.Init.WordLength = UART_WORDLENGTH_8B;
    huart_rs485.Init.StopBits = UART_STOPBITS_1;
    huart_rs485.Init.Parity = UART_PARITY_NONE;
    huart_rs485.Init.Mode = UART_MODE_TX_RX;
    huart_rs485.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart_rs485.Init.OverSampling = UART_OVERSAMPLING_16;
    huart_rs485.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart_rs485.Init.ClockPrescaler = UART_PRESCALER_DIV1;
    huart_rs485.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    if (HAL_UART_Init(&huart_rs485) != HAL_OK) {
        abort();
    }
    if (HAL_UARTEx_SetTxFifoThreshold(&huart_rs485, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK) {
        abort();
    }
    if (HAL_UARTEx_SetRxFifoThreshold(&huart_rs485, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK) {
        abort();
    }
    if (HAL_UARTEx_DisableFifoMode(&huart_rs485) != HAL_OK) {
        abort();
    }
    if (HAL_RS485Ex_Init(&huart_rs485, UART_DE_POLARITY_HIGH, 0x1f, 0x1f) != HAL_OK) {
        abort();
    }
}

void mmu_init() {
    huart_mmu.Instance = USART2;
    huart_mmu.Init.BaudRate = 115'200;
    huart_mmu.Init.WordLength = UART_WORDLENGTH_8B;
    huart_mmu.Init.StopBits = UART_STOPBITS_1;
    huart_mmu.Init.Parity = UART_PARITY_NONE;
    huart_mmu.Init.Mode = UART_MODE_TX_RX;
    huart_mmu.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart_mmu.Init.OverSampling = UART_OVERSAMPLING_16;
    huart_mmu.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart_mmu.Init.ClockPrescaler = UART_PRESCALER_DIV1;
    huart_mmu.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    if (HAL_UART_Init(&huart_mmu) != HAL_OK) {
        abort();
    }
    if (HAL_UARTEx_SetTxFifoThreshold(&huart_mmu, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK) {
        abort();
    }
    if (HAL_UARTEx_SetRxFifoThreshold(&huart_mmu, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK) {
        abort();
    }
    if (HAL_UARTEx_DisableFifoMode(&huart_mmu) != HAL_OK) {
        abort();
    }
    HAL_UARTEx_ReceiveToIdle_IT(&huart_mmu, (uint8_t *)&rx_byte_mmu, 1);
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
        .Pin = GPIO_PIN_6 | GPIO_PIN_7,
        .Mode = GPIO_MODE_AF_PP,
        .Pull = GPIO_NOPULL,
        .Speed = GPIO_SPEED_FREQ_LOW,
        .Alternate = GPIO_AF2_TIM3,
    };
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
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

    // auto-reload value
    TIM2->ARR = 255;

    // enable counter
    TIM2->CR1 |= TIM_CR1_CEN;
}

static void tim3_init() {
    // reset peripheral
    __HAL_RCC_TIM3_CLK_ENABLE();
    __HAL_RCC_TIM3_FORCE_RESET();
    __HAL_RCC_TIM3_RELEASE_RESET();

    // channel 1 settings
    TIM3->CCMR1 |= (TIM_OCMODE_PWM1 << 0) | TIM_CCMR1_OC1PE;

    // channel 2 settings
    TIM3->CCMR1 |= (TIM_OCMODE_PWM2 << 8) | TIM_CCMR1_OC2PE;

    // channel 3 settings
    TIM3->CCMR2 |= (TIM_OCMODE_PWM2 << 0) | TIM_CCMR2_OC3PE;

    // enable output of channels
    TIM3->CCER = TIM_CCER_CC1E | TIM_CCER_CC2E | TIM_CCER_CC3E;

    // auto-reload value
    TIM3->ARR = 255;

    // enable counter
    TIM3->CR1 |= TIM_CR1_CEN;
}

ADC_HandleTypeDef hadc1;

static void MX_ADC1_Init(void) {
    hadc1.Instance = ADC1;
    hadc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
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
    const auto calib_val = HAL_ADC_GetValue(&hadc1);
    HAL_ADCEx_Calibration_SetValue(&hadc1, single_diff, calib_val);
}

I2C_HandleTypeDef hi2c;

extern "C" void HAL_I2C_MspInit(I2C_HandleTypeDef *hi2c) {
    if (hi2c->Instance == I2C2) {
        __HAL_RCC_I2C2_CONFIG(RCC_I2C2CLKSOURCE_PCLK1);
        __HAL_RCC_GPIOB_CLK_ENABLE();
        /**I2C2 GPIO Configuration
        PB10     ------> I2C2_SCL
        PB13     ------> I2C2_SDA
        */
        GPIO_InitTypeDef GPIO_InitStruct = {
            .Pin = GPIO_PIN_10 | GPIO_PIN_13,
            .Mode = GPIO_MODE_AF_OD,
            .Pull = GPIO_NOPULL,
            .Speed = GPIO_SPEED_FREQ_LOW,
            .Alternate = GPIO_AF4_I2C2,
        };
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

        /* Peripheral clock enable */
        __HAL_RCC_I2C2_CLK_ENABLE();
    }
}

static void i2c2_init() {
    hi2c.Instance = I2C2;
    hi2c.Init.OwnAddress1 = 0;
    hi2c.Init.Timing = 0x00707CBB; // Generated by stm32 cube IDE when using default settings 🤷
    hi2c.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c.Init.OwnAddress2 = 0;
    hi2c.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&hi2c) != HAL_OK) {
        abort();
    }
}

namespace TCA6408A {

enum class Register : uint8_t {
    Input = 0,
    Output = 1,
    Polarity = 2,
    Config = 3,
};

void write_register(Register reg, uint8_t value) {
    uint16_t device_address = 0x40;
    uint8_t data[2] = { uint8_t(reg), value };
    if (HAL_I2C_Master_Transmit(&hi2c, device_address, data, sizeof(data), HAL_MAX_DELAY) != HAL_OK) {
        abort();
    }
}

} // namespace TCA6408A

static void enable_fans() {
    // configure all pins as output
    TCA6408A::write_register(TCA6408A::Register::Config, 0x00);

    // set all pins, we don't need to enable them individually at the moment
    TCA6408A::write_register(TCA6408A::Register::Output, 0xff);
}

static void mmu_pins_init() {
    GPIO_InitTypeDef GPIO_InitStruct;
    __HAL_RCC_GPIOC_CLK_ENABLE();
    GPIO_InitStruct.Pin = GPIO_PIN_13 | GPIO_PIN_14;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = 0;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

void hal::init() {
    HAL_Init();
    hal::overclock();
    tim2_init();
    tim3_init();
    tim2_postinit();
    tim3_postinit();
    MX_ADC1_Init();
    rs485_init();
    mmu_init();
    i2c2_init();
    enable_fans();
    mmu_pins_init();
    mmu::nreset_pin_set(false);
    mmu::power_pin_set(true);
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

uint32_t temperature_raw = 0;

void hal::step() {
    // Until we have a non-blocking DMA or interrupt based ADC, we do this
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
    temperature_raw = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);
}

// Until we implement reading fan tachometers,
// we stub it out based on if they should be spinning or not, so
// the spin-up algorithm on the printer side does something sane.
static bool fan1_fan2_enabled = false;

// fan1 and fan2 are connected to the same pwm signal
static void fan1_fan2_set_pwm(hal::DutyCycle duty_cycle) {
    TIM3->CCR2 = duty_cycle;
    fan1_fan2_enabled = duty_cycle >= 1;
}

void hal::fan1::set_pwm(DutyCycle duty_cycle) {
    fan1_fan2_set_pwm(duty_cycle);
}

uint32_t hal::fan1::get_raw() {
    return fan1_fan2_enabled ? 500 : 0;
}

void hal::fan2::set_pwm(DutyCycle duty_cycle) {
    fan1_fan2_set_pwm(duty_cycle);
}

uint32_t hal::fan2::get_raw() {
    return fan1_fan2_enabled ? 500 : 0;
}

void hal::fan3::set_pwm(DutyCycle duty_cycle) {
    TIM3->CCR3 = duty_cycle;
}

uint32_t hal::fan3::get_raw() {
    // TODO
    return 0;
}

void hal::w_led::set_pwm(DutyCycle duty_cycle) {
    TIM3->CCR1 = duty_cycle;
}

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

void hal::rs485::start_receiving() {
    HAL_UART_TxCpltCallback(&huart_rs485);
}

std::span<std::byte> hal::rs485::receive() {
    tx_semaphore_rs485.acquire();
    return { rx_buf_rs485, rx_len_rs485 };
}

void hal::rs485::transmit_and_then_start_receiving(std::span<std::byte> payload) {
    HAL_UART_Transmit_IT(&huart_rs485, (uint8_t *)payload.data(), payload.size());
}

void hal::mmu::transmit(std::span<const std::byte> payload) {
    HAL_UART_Transmit_IT(&huart_mmu, (const uint8_t *)payload.data(), payload.size());
    tx_semaphore_mmu.acquire();
}

std::span<std::byte> hal::mmu::receive(std::span<std::byte> buffer) {
    return rx_mmu_buffer.receive(buffer);
}

void hal::mmu::flush() {
    std::byte buf[8];
    while (!receive(buf).empty()) {
    }
}

void hal::mmu::power_pin_set(bool b) {
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_14, b ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void hal::mmu::nreset_pin_set(bool b) {
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, b ? GPIO_PIN_SET : GPIO_PIN_RESET);
}
