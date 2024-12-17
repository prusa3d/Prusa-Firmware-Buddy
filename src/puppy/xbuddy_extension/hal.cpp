/// @file
#include "hal.hpp"

#include "hal_clock.hpp"
#include <bitset>
#include <freertos/binary_semaphore.hpp>
#include <freertos/stream_buffer.hpp>
#include <freertos/timing.hpp>
#include <stm32h5xx_hal.h>
#include <stm32h5xx_ll_gpio.h>

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

static void tim1_postinit() {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**TIM2 GPIO Configuration
    PA8     ------> TIM1_CH1
    PA9     ------> TIM1_CH2
    PA10    ------> TIM1_CH3
    */
    constexpr GPIO_InitTypeDef GPIO_InitStruct {
        .Pin = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10,
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

static void tim1_init() {
    // reset peripheral
    __HAL_RCC_TIM1_CLK_ENABLE();
    __HAL_RCC_TIM1_FORCE_RESET();
    __HAL_RCC_TIM1_RELEASE_RESET();

    // input mode, without remapping
    constexpr const uint32_t capture_compare_selection = 0b01;

    // no filter, sampling is done at fDTS; this could be changed if we start getting false edges
    constexpr const uint32_t input_capture_filter = 0b0000;

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
    TIM2->PSC = 10 - 1;

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

    // channel 1 settings
    TIM3->CCMR1 |= (TIM_OCMODE_PWM1 << 0) | TIM_CCMR1_OC1PE;

    // channel 2 settings
    TIM3->CCMR1 |= (TIM_OCMODE_PWM2 << 8) | TIM_CCMR1_OC2PE;

    // channel 3 settings
    TIM3->CCMR2 |= (TIM_OCMODE_PWM2 << 0) | TIM_CCMR2_OC3PE;

    // enable output of channels
    TIM3->CCER = TIM_CCER_CC1E | TIM_CCER_CC2E | TIM_CCER_CC3E;

    // 6 MHz clock (30 MHz peripheral clock, *2 to timer, /10 prescaler)
    TIM3->PSC = 10 - 1;

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

static constexpr uint32_t diff32(uint32_t prev, uint32_t curr) {
    return (curr >= prev)
        ? (curr - prev)
        : (0xffffffff - prev + curr);
}
static constexpr uint32_t diff16(uint32_t prev, uint32_t curr) {
    return (curr >= prev)
        ? (curr - prev)
        : (0xffff - prev + curr);
}
static_assert(diff16(0x0000, 0x0000) == 0x0);
static_assert(diff16(0x0000, 0x0008) == 0x8);
static_assert(diff16(0x0008, 0x0008) == 0x0);
static_assert(diff16(0x0008, 0x0010) == 0x8);
static_assert(diff16(0x0008, 0x0010) == 0x8);
static_assert(diff16(0xfff8, 0x0001) == 0x8);

// This number is incremented whenever TIM1 overflows.
static uint32_t tim1_generation = 0;

extern "C" void TIM1_UP_IRQHandler() {
    const uint32_t SR = TIM1->SR;
    TIM1->SR = SR & ~(TIM_SR_UIF);
    if (SR & TIM_SR_UIF) {
        ++tim1_generation;
    }
}

class Tim1ChannelData {
private:
    // Since TIM1 is a 16-bit timer, we can afford to store both previous
    // and current values into a single machine word.
    // This greatly simplifies IRQ handler.
    uint32_t prev_curr = 0;
    uint32_t generation = 0;

public:
    void update(uint32_t ccr) {
        prev_curr = (prev_curr << 16) | ccr;
        generation = tim1_generation;
    }

    uint32_t period() const {
        return diff32(generation, tim1_generation) > 3 ? 0 : diff16(prev_curr >> 16, prev_curr & 0xffff);
    }
};
Tim1ChannelData tim1_cc1;
Tim1ChannelData tim1_cc2;
Tim1ChannelData tim1_cc3;

extern "C" void TIM1_CC_IRQHandler() {
    const uint32_t SR = TIM1->SR;
    TIM1->SR = SR & ~(TIM_SR_CC1IF | TIM_SR_CC2IF | TIM_SR_CC3IF);
    if (SR & TIM_SR_CC1IF) {
        tim1_cc1.update(TIM1->CCR1);
    }
    if (SR & TIM_SR_CC2IF) {
        tim1_cc2.update(TIM1->CCR2);
    }
    if (SR & TIM_SR_CC3IF) {
        tim1_cc3.update(TIM1->CCR3);
    }
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

static constexpr const uint8_t expander_pin_mmu_power = 1 << 2;
static constexpr const uint8_t expander_pin_fan3 = 1 << 3;
static constexpr const uint8_t expander_pin_fan2 = 1 << 4;
static constexpr const uint8_t expander_pin_fan1 = 1 << 5;
static uint8_t expander_pins = 0;

static void init_expander() {
    // setup all signals before configuring pins as output
    TCA6408A::write_register(TCA6408A::Register::Output, expander_pins);

    // configure all pins as output
    TCA6408A::write_register(TCA6408A::Register::Config, 0x00);
}

static void enable_fans() {
    expander_pins |= expander_pin_fan1 | expander_pin_fan2 | expander_pin_fan3;
    TCA6408A::write_register(TCA6408A::Register::Output, expander_pins);
}

static void mmu_pins_init() {
    GPIO_InitTypeDef GPIO_InitStruct;
    __HAL_RCC_GPIOC_CLK_ENABLE();
    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = 0;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

void usb_pins_init() {
    constexpr GPIO_InitTypeDef GPIO_InitStruct {
        .Pin = GPIO_PIN_2,
        .Mode = GPIO_MODE_OUTPUT_PP,
        .Pull = GPIO_NOPULL,
        .Speed = GPIO_SPEED_FREQ_LOW,
        .Alternate = 0,
    };
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

static void pub_init() {
    __HAL_RCC_GPIOC_CLK_ENABLE();
    constexpr GPIO_InitTypeDef GPIO_InitStruct {
        .Pin = GPIO_PIN_15,
        .Mode = GPIO_MODE_OUTPUT_PP,
        .Pull = GPIO_NOPULL,
        .Speed = GPIO_SPEED_FREQ_LOW,
        .Alternate = 0,
    };
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

static void pub_enable() {
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_15, GPIO_PIN_SET);
}

static void filament_sensor_pins_init() {
    constexpr GPIO_InitTypeDef GPIO_InitStruct {
        .Pin = GPIO_PIN_5,
        .Mode = GPIO_MODE_INPUT,
        .Pull = GPIO_PULLDOWN,
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
    rs485_init();
    mmu_init();
    i2c2_init();
    init_expander();
    enable_fans();
    mmu_pins_init();
    mmu::nreset_pin_set(false);
    usb_pins_init();
    pub_init();
    pub_enable();
    filament_sensor_pins_init();
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

static uint8_t filament_sensor_measuring_phase = 0;

/// FS readout at each phase
static std::bitset<4> filament_sensor_raw;

static hal::filament_sensor::State filament_sensor_state = hal::filament_sensor::State::uninitialized;

static size_t filament_sensor_last_millis = 0;

static void step_temperature_adc() {
    // Until we have a non-blocking DMA or interrupt based ADC, we do this
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
    temperature_raw = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);
}

static void step_filament_sensor() {
    const auto now = freertos::millis();

    // Don't update that often, the pin readouts takes some serious time to stabilize
    if (now - filament_sensor_last_millis <= 10) {
        return;
    }

    filament_sensor_last_millis = now;
    filament_sensor_raw[filament_sensor_measuring_phase] = (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_5) == GPIO_PIN_SET);
    filament_sensor_measuring_phase = (filament_sensor_measuring_phase + 1) % 4;

    // Set up the pull for the next phase, use the time between phases to stabilize the readout
    LL_GPIO_SetPinPull(GPIOA, GPIO_PIN_5, filament_sensor_measuring_phase % 2 ? GPIO_PULLUP : GPIO_PULLDOWN);

    switch (filament_sensor_raw.to_ulong()) {
    case 0b1111:
        filament_sensor_state = hal::filament_sensor::State::has_filament;
        break;

    case 0b0000:
        filament_sensor_state = hal::filament_sensor::State::no_filament;
        break;

    case 0b1010:
        // The readout followed exactly the pullup changes -> there's nothing connected
        filament_sensor_state = hal::filament_sensor::State::disconnected;
        break;

    default:
        // The filament could have been inserted/removed between the phases, wait for definitive values
        break;
    }
}

void hal::step() {
    step_temperature_adc();
    step_filament_sensor();
}

static uint32_t tim1_period_to_rpm(uint32_t period) {
    if (period == 0) {
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
    return tim1_period_to_rpm(tim1_cc1.period());
}

void hal::fan2::set_pwm(DutyCycle duty_cycle) {
    fan1_fan2_set_pwm(duty_cycle);
}

uint32_t hal::fan2::get_rpm() {
    // When debugging with just one fan, it might be useful to uncomment
    // following line because pwm is shared and motherboard goes crazy
    // when only one of the fans is spinning...
    // return fan1::get_rpm();
    return tim1_period_to_rpm(tim1_cc2.period());
}

void hal::fan3::set_pwm(DutyCycle duty_cycle) {
    TIM3->CCR3 = duty_cycle;
}

uint32_t hal::fan3::get_rpm() {
    return tim1_period_to_rpm(tim1_cc3.period());
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

hal::filament_sensor::State hal::filament_sensor::get() {
    return filament_sensor_state;
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
    if (b) {
        expander_pins |= expander_pin_mmu_power;
    } else {
        expander_pins &= ~expander_pin_mmu_power;
    }
    TCA6408A::write_register(TCA6408A::Register::Output, expander_pins);
}

void hal::mmu::nreset_pin_set(bool b) {
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, b ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

bool hal::mmu::power_pin_get() {
    return expander_pins | expander_pin_mmu_power;
}

bool hal::mmu::nreset_pin_get() {
    return HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == GPIO_PIN_SET;
}

void hal::usb::power_pin_set(bool enabled) {
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, enabled ? GPIO_PIN_SET : GPIO_PIN_RESET);
}
