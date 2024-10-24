///@file
#include "hal.hpp"

#include <stm32h5xx_hal.h>
#include <freertos/queue.hpp>

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

#define D_RS485_FLOW_CONTROL GPIOC, GPIO_PIN_14

static UART_HandleTypeDef huart;
static std::byte rx_buf[256];
static volatile size_t rx_len;

// FIXME: Use freertos::BinarySemaphore after we fix the handle issue
//        when using MPU, as we did for the queue. We are always having
//        only one receive in flight.
struct Element {};
static freertos::Queue<Element, 1> queue;

extern "C" void USART1_IRQHandler(void) {
    HAL_UART_IRQHandler(&huart);
}

extern "C" void HAL_UART_MspInit(UART_HandleTypeDef *huart) {
    GPIO_InitTypeDef GPIO_InitStruct {};
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct {};
    if (huart->Instance == USART1) {
        PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USART1;
        PeriphClkInitStruct.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK) {
            abort();
        }
        __HAL_RCC_USART1_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();
        GPIO_InitStruct.Pin = GPIO_PIN_14 | GPIO_PIN_15;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF4_USART1;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
        HAL_NVIC_SetPriority(USART1_IRQn, 5, 0);
        HAL_NVIC_EnableIRQ(USART1_IRQn);
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
        Element payload;
        queue.send_from_isr(payload);
    }
}

extern "C" void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
    HAL_GPIO_WritePin(D_RS485_FLOW_CONTROL, GPIO_PIN_RESET);
    HAL_UARTEx_ReceiveToIdle_IT(huart, (uint8_t *)rx_buf, sizeof(rx_buf));
}

void rs485_init() {
    huart.Instance = USART1;
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

        __HAL_RCC_GPIOA_CLK_ENABLE();
        /**ADC1 GPIO Configuration
        PA4     ------> ADC1_INP18
        */
        GPIO_InitStruct.Pin = GPIO_PIN_4;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
}

static void init_gpio_pin(GPIO_TypeDef *port, uint32_t pin, uint32_t mode = GPIO_MODE_OUTPUT_PP) {
    HAL_GPIO_WritePin(port, pin, GPIO_PIN_RESET);
    GPIO_InitTypeDef GPIO_InitStruct {
        .Pin = pin,
        .Mode = mode,
        .Pull = GPIO_NOPULL,
        .Speed = GPIO_SPEED_FREQ_LOW,
        .Alternate = 0,
    };
    HAL_GPIO_Init(port, &GPIO_InitStruct);
}

static void tim3_postinit() {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**TIM3 GPIO Configuration
    PA6     ------> TIM3_CH1
    PA7     ------> TIM3_CH2
    PA8     ------> TIM3_CH3
    */
    constexpr GPIO_InitTypeDef GPIO_InitStruct {
        .Pin = GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8,
        .Mode = GPIO_MODE_AF_PP,
        .Pull = GPIO_NOPULL,
        .Speed = GPIO_SPEED_FREQ_LOW,
        .Alternate = GPIO_AF2_TIM3,
    };
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
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

#define D_FAN2_EN            GPIOB, GPIO_PIN_10
#define D_FAN1_EN            GPIOB, GPIO_PIN_13
#define D_RS485_FLOW_CONTROL GPIOC, GPIO_PIN_14

static void init_gpio_pins() {
    __HAL_RCC_GPIOB_CLK_ENABLE();
    init_gpio_pin(D_FAN2_EN);
    init_gpio_pin(D_FAN1_EN);
    __HAL_RCC_GPIOC_CLK_ENABLE();
    init_gpio_pin(D_RS485_FLOW_CONTROL);
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

    ADC_ChannelConfTypeDef sConfig = {};
    sConfig.Channel = ADC_CHANNEL_18;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_2CYCLES_5;
    sConfig.SingleDiff = ADC_SINGLE_ENDED;
    sConfig.OffsetNumber = ADC_OFFSET_NONE;
    sConfig.Offset = 0;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
        abort();
    }
}

void hal::init() {
    HAL_Init();
    SystemClock_Config();
    init_gpio_pins();
    tim3_init();
    tim3_postinit();
    MX_ADC1_Init();
    rs485_init();
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

// The v1 board has 3-pin fans, reading fan's PWM is hard, so we stub it out
// for now for this revision, based on if they should be spinning or not, so
// the spin-up algorithm on the printer side does something sane.
static bool fan1_enabled = false;
static bool fan2_enabled = false;

void hal::fan1::set_enabled(bool b) {
    HAL_GPIO_WritePin(D_FAN1_EN, b ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void hal::fan1::set_pwm(DutyCycle duty_cycle) {
    TIM3->CCR3 = duty_cycle;
    fan1_enabled = duty_cycle >= 1;
}

uint32_t hal::fan1::get_raw() {
    return fan1_enabled ? 500 : 0;
}

void hal::fan2::set_enabled(bool b) {
    HAL_GPIO_WritePin(D_FAN2_EN, b ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void hal::fan2::set_pwm(DutyCycle duty_cycle) {
    TIM3->CCR2 = duty_cycle;
    fan2_enabled = duty_cycle >= 1;
}

uint32_t hal::fan2::get_raw() {
    return fan2_enabled ? 500 : 0;
}

void hal::fan3::set_enabled(bool b) {
    // TODO
    // xBuddyExtension rev.1 doesn't support fan3
    (void)b;
}

void hal::fan3::set_pwm(DutyCycle duty_cycle) {
    // TODO
    // xBuddyExtension rev.1 doesn't support fan3
    (void)duty_cycle;
}

uint32_t hal::fan3::get_raw() {
    // TODO
    return 0;
}

void hal::w_led::set_pwm(DutyCycle duty_cycle) {
    TIM3->CCR1 = duty_cycle;
}

void hal::rgbw_led::set_r_pwm(DutyCycle duty_cycle) {
    // TODO
    (void)duty_cycle;
}

void hal::rgbw_led::set_g_pwm(DutyCycle duty_cycle) {
    // TODO
    (void)duty_cycle;
}

void hal::rgbw_led::set_b_pwm(DutyCycle duty_cycle) {
    // TODO
    (void)duty_cycle;
}

void hal::rgbw_led::set_w_pwm(DutyCycle duty_cycle) {
    // TODO
    (void)duty_cycle;
}

uint32_t hal::temperature::get_raw() {
    return temperature_raw;
}

void hal::rs485::start_receiving() {
    HAL_UART_TxCpltCallback(&huart);
}

std::span<std::byte> hal::rs485::receive() {
    Element payload;
    queue.receive(payload);
    return { rx_buf, rx_len };
}

void hal::rs485::transmit_and_then_start_receiving(std::span<std::byte> payload) {
    HAL_GPIO_WritePin(D_RS485_FLOW_CONTROL, GPIO_PIN_SET);
    HAL_UART_Transmit_IT(&huart, (uint8_t *)payload.data(), payload.size());
}
