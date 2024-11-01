///@file
#include "hal.hpp"

#include <stm32h5xx_hal.h>
#include <freertos/binary_semaphore.hpp>

void SystemClock_Config(void) {
    /** Configure the main internal regulator output voltage
     */
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

    while (!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {
    }

    /** Initializes the RCC Oscillators according to the specified parameters
     * in the RCC_OscInitTypeDef structure.
     */
    constexpr RCC_OscInitTypeDef RCC_OscInitStruct = {
        .OscillatorType = RCC_OSCILLATORTYPE_HSI,
        .HSEState = 0,
        .LSEState = 0,
        .HSIState = RCC_HSI_ON,
        .HSIDiv = RCC_HSI_DIV2,
        .HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT,
        .LSIState = 0,
        .CSIState = 0,
        .CSICalibrationValue = 0,
        .HSI48State = 0,
        .PLL = {
            .PLLState = RCC_PLL_NONE,
            .PLLSource = 0,
            .PLLM = 0,
            .PLLN = 0,
            .PLLP = 0,
            .PLLQ = 0,
            .PLLR = 0,
            .PLLRGE = 0,
            .PLLVCOSEL = 0,
            .PLLFRACN = 0,
        },
    };
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        abort();
    }

    /** Initializes the CPU, AHB and APB buses clocks
     */
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {
        .ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
            | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2
            | RCC_CLOCKTYPE_PCLK3,
        .SYSCLKSource = RCC_SYSCLKSOURCE_HSI,
        .AHBCLKDivider = RCC_SYSCLK_DIV1,
        .APB1CLKDivider = RCC_HCLK_DIV1,
        .APB2CLKDivider = RCC_HCLK_DIV1,
        .APB3CLKDivider = RCC_HCLK_DIV1,
    };

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK) {
        abort();
    }

    /** Configure the programming delay
     */
    __HAL_FLASH_SET_PROGRAM_DELAY(FLASH_PROGRAMMING_DELAY_0);
}

static UART_HandleTypeDef huart;
static std::byte rx_buf[256];
static volatile size_t rx_len;
static freertos::BinarySemaphore tx_semaphore;

extern "C" void USART3_IRQHandler(void) {
    HAL_UART_IRQHandler(&huart);
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
}

extern "C" void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
    // TODO
    (void)huart;
}

extern "C" void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size) {
    if (size == 0) {
        HAL_UARTEx_ReceiveToIdle_IT(huart, (uint8_t *)rx_buf, sizeof(rx_buf));
    } else {
        rx_len = size;
        long task_woken = tx_semaphore.release_from_isr();
        // TODO We could wake up correct task here, but there is no freertos:: wrapper at the moment
        (void)task_woken;
    }
}

extern "C" void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
    HAL_UARTEx_ReceiveToIdle_IT(huart, (uint8_t *)rx_buf, sizeof(rx_buf));
}

void rs485_init() {
    huart.Instance = USART3;
    huart.Init.BaudRate = 230'400;
    huart.Init.WordLength = UART_WORDLENGTH_8B;
    huart.Init.StopBits = UART_STOPBITS_1;
    huart.Init.Parity = UART_PARITY_NONE;
    huart.Init.Mode = UART_MODE_TX_RX;
    huart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart.Init.OverSampling = UART_OVERSAMPLING_16;
    huart.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart.Init.ClockPrescaler = UART_PRESCALER_DIV1;
    huart.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    if (HAL_UART_Init(&huart) != HAL_OK) {
        abort();
    }
    if (HAL_UARTEx_SetTxFifoThreshold(&huart, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK) {
        abort();
    }
    if (HAL_UARTEx_SetRxFifoThreshold(&huart, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK) {
        abort();
    }
    if (HAL_UARTEx_DisableFifoMode(&huart) != HAL_OK) {
        abort();
    }
    if (HAL_RS485Ex_Init(&huart, UART_DE_POLARITY_HIGH, 0x1f, 0x1f) != HAL_OK) {
        abort();
    }
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

void hal::init() {
    HAL_Init();
    SystemClock_Config();
    tim2_init();
    tim3_init();
    tim2_postinit();
    tim3_postinit();
    MX_ADC1_Init();
    rs485_init();
    i2c2_init();
    enable_fans();
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
    HAL_UART_TxCpltCallback(&huart);
}

std::span<std::byte> hal::rs485::receive() {
    tx_semaphore.acquire();
    return { rx_buf, rx_len };
}

void hal::rs485::transmit_and_then_start_receiving(std::span<std::byte> payload) {
    HAL_UART_Transmit_IT(&huart, (uint8_t *)payload.data(), payload.size());
}
