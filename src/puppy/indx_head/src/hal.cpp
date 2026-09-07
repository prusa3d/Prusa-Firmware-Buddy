#include "hal.hpp"

#include "critical_section.hpp"
#include "hal_crc.hpp"
#include "hal_spi.hpp"
#include "heater.hpp"
#include "spi_task.hpp"
#include "watchdog.hpp"

#include <stm32c0xx_hal.h>
#include <stm32c0xx_ll_tim.h>
#include <stm32c0xx_ll_gpio.h>
#include <stm32c0xx_ll_system.h>

#include <bsod/bsod.h>
#include <freertos/binary_semaphore.hpp>
#include <indx_head/errors.hpp>

freertos::BinarySemaphore hal::peripherals::adc_semaphore;

extern "C" {
#include <FreeRTOSConfig.h>
}
static_assert(configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY == 2);
static constexpr uint32_t system_clock = 48'000'000;

using indx_head::errors::FaultStatusMask;

FaultStatusMask hal::get_fault_status() {
    return static_cast<FaultStatusMask>(PWR->BKP0R);
}

void hal::set_fault_status(FaultStatusMask fault) {
    CriticalSection cs;
    PWR->BKP0R |= static_cast<uint32_t>(fault);
}

void hal::clear_fault_status(FaultStatusMask fault) {
    CriticalSection cs;
    PWR->BKP0R &= ~static_cast<uint32_t>(fault);
}

static void clear_reset_flags() {
    const uint32_t RCC_CSR2 = RCC->CSR2;
    if (RCC_CSR2 & RCC_CSR2_IWDGRSTF) {
        hal::set_fault_status(FaultStatusMask::watchdog_reset);
    }
    if (RCC_CSR2 & RCC_CSR2_PINRSTF) {
        hal::set_fault_status(FaultStatusMask::pin_reset);
    }
    if (RCC_CSR2 & RCC_CSR2_PWRRSTF) {
        hal::set_fault_status(FaultStatusMask::power_reset);
    }
    RCC->CSR2 = RCC_CSR2_RMVF;
}

void hal::panic(FaultStatusMask fault) {
    set_fault_status(fault);
    set_safe_state();
    while (1) {
    }
    // NVIC_SystemReset();
}

void hal::set_safe_state() {
    set_ind_heater_pwr(false); // Disable induction heater for safety
    tim::set_printfan_pwm(255);
    tim::set_heatbreak_fan_pwm(255);
}

namespace hal {
enum ADC_Config {
    NONE,
    DEFAULT,
    INDUCTION_HEATER
};
auto current_adc_config = ADC_Config::NONE;
void init_clock();
void init_gpio();
void init_rs485();
void init_spi();
void init_i2c();
void init_adc();
void init_tim1();
void init_tim3();
void init_tim16();
} // namespace hal

namespace hal::peripherals {
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;
DMA_HandleTypeDef hdma_i2c1_rx;
DMA_HandleTypeDef hdma_i2c1_tx;
I2C_HandleTypeDef hi2c1;
TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim16;
UART_HandleTypeDef huart2;
} // namespace hal::peripherals

void hal::init() {
    HAL_Init();
    LL_FLASH_EnablePrefetch();
    hal::init_clock();
    clear_reset_flags();
    hal::init_gpio();
    hal::init_rs485();
    hal::init_i2c();
    hal::init_adc();
    hal::init_tim1();
    hal::init_tim3();
    hal::init_tim16();
    hal::set_ind_heater_pwr(true); // Enable induction heater power // INDX_HEAD_TODO: Enable/disable depending on heating target
    hal::init_spi();
    hal::crc::init();
}

void hal::set_ind_heater_pwr(bool enabled) {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, enabled ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void hal::init_clock() {
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();

    __HAL_FLASH_SET_LATENCY(FLASH_LATENCY_1);

    static constexpr RCC_OscInitTypeDef RCC_OscInitStruct {
        .OscillatorType = RCC_OSCILLATORTYPE_LSI | RCC_OSCILLATORTYPE_HSE,
        .HSEState = RCC_HSE_ON,
        .LSEState = RCC_LSE_OFF,
        .HSIState = RCC_HSI_OFF,
        .HSIDiv = RCC_HSI_DIV1,
        .HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT,
        .LSIState = RCC_LSI_ON,
    };
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        bsod_system();
    }

    static constexpr RCC_ClkInitTypeDef RCC_ClkInitStruct {
        .ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1,
        .SYSCLKSource = RCC_SYSCLKSOURCE_HSE,
        .SYSCLKDivider = RCC_SYSCLK_DIV1,
        .AHBCLKDivider = RCC_HCLK_DIV1,
        .APB1CLKDivider = RCC_APB1_DIV1,
    };

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK) {
        bsod_system();
    }
}

void hal::init_gpio() {
    __HAL_RCC_GPIOA_CLK_ENABLE();

    // Induction heater power control pin
    {
        static constexpr GPIO_InitTypeDef GPIO_InitStruct {
            .Pin = GPIO_PIN_5,
            .Mode = GPIO_MODE_OUTPUT_PP,
            .Pull = GPIO_NOPULL,
            .Speed = GPIO_SPEED_FREQ_LOW,
            .Alternate = 0,
        };
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
}

void hal::init_rs485() {
    using namespace peripherals;

    __HAL_RCC_USART2_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**USART2 GPIO Configuration
    PA2     ------> USART2_TX
    PA3     ------> USART2_RX
    PB9     ------> USART2_DE_RE
    */

    {
        static constexpr GPIO_InitTypeDef GPIO_InitStruct {
            .Pin = GPIO_PIN_2 | GPIO_PIN_3,
            .Mode = GPIO_MODE_AF_PP,
            .Pull = GPIO_NOPULL,
            .Speed = GPIO_SPEED_FREQ_LOW,
            .Alternate = GPIO_AF1_USART2,
        };
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }

    {
        static constexpr GPIO_InitTypeDef GPIO_InitStruct {
            .Pin = GPIO_PIN_9,
            .Mode = GPIO_MODE_OUTPUT_PP,
            .Pull = GPIO_NOPULL,
            .Speed = GPIO_SPEED_FREQ_LOW,
            .Alternate = 0,
        };
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    }

    huart2.Instance = USART2;
    huart2.Init.BaudRate = 230400;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart2.Init.ClockPrescaler = UART_PRESCALER_DIV1;
    huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    if (HAL_UART_Init(&huart2) != HAL_OK) {
        bsod_system();
    }

    HAL_NVIC_SetPriority(USART2_IRQn, ISR_PRIORITY_DEFAULT - 1, 0);
    HAL_NVIC_EnableIRQ(USART2_IRQn);

    hal::rs485::init();
}

void hal::init_spi() {
    using namespace peripherals;

    static constexpr RCC_PeriphCLKInitTypeDef PeriphClkInit {
        .PeriphClockSelection = RCC_PERIPHCLK_I2S1,
        .HSIKerClockDivider = RCC_HSIKER_DIV1,
        .Usart1ClockSelection = 0,
        .I2c1ClockSelection = 0,
        .I2s1ClockSelection = RCC_I2S1CLKSOURCE_SYSCLK,
        .Fdcan1ClockSelection = 0,
        .AdcClockSelection = 0,
        .RTCClockSelection = 0,
    };
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
        bsod_system();
    }

    __HAL_RCC_SPI1_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    /**SPI1 GPIO Configuration
    PA11 ------> SPI_MISO
    PA12 ------> SPI_MOSI
    PB3  ------> SPI_SCK
    PA15 ------> SPI_ADC_NSS
    PB0  ------> SPI_ACCEL_INT
    PC6  ------> SPI_ACCEL_CS
    PA10 ------> SPI_ADC_CLK_SRC
    PB4  ------> SPI_ADC_INT
    */
    { // MISO, MOSI
        static constexpr GPIO_InitTypeDef GPIO_InitStruct {
            .Pin = GPIO_PIN_11 | GPIO_PIN_12,
            .Mode = GPIO_MODE_AF_PP,
            .Pull = GPIO_NOPULL,
            .Speed = GPIO_SPEED_FREQ_LOW,
            .Alternate = GPIO_AF0_SPI1,
        };
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }

    { // SCLK
        static constexpr GPIO_InitTypeDef GPIO_InitStruct {
            .Pin = GPIO_PIN_3,
            .Mode = GPIO_MODE_AF_PP,
            .Pull = GPIO_NOPULL,
            .Speed = GPIO_SPEED_FREQ_LOW,
            .Alternate = GPIO_AF0_SPI1,
        };
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    }

    { // ADC_NSS
        static constexpr GPIO_InitTypeDef GPIO_InitStruct {
            .Pin = GPIO_PIN_15,
            .Mode = GPIO_MODE_OUTPUT_PP,
            .Pull = GPIO_NOPULL,
            .Speed = GPIO_SPEED_FREQ_LOW,
            .Alternate = 0,
        };
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_SET);
    }

    { // ACCEL_INT
        static constexpr GPIO_InitTypeDef GPIO_InitStruct {
            .Pin = GPIO_PIN_0,
            .Mode = GPIO_MODE_IT_RISING,
            .Pull = GPIO_NOPULL,
            .Speed = GPIO_SPEED_FREQ_LOW,
            .Alternate = 0,
        };
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    }

    { // ACCEL_CS
        static constexpr GPIO_InitTypeDef GPIO_InitStruct {
            .Pin = GPIO_PIN_6,
            .Mode = GPIO_MODE_OUTPUT_PP,
            .Pull = GPIO_NOPULL,
            .Speed = GPIO_SPEED_FREQ_LOW,
            .Alternate = 0,
        };
        HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_SET);
    }

    { // ADC_CLK_SRC
        static constexpr GPIO_InitTypeDef GPIO_InitStruct {
            .Pin = GPIO_PIN_10,
            .Mode = GPIO_MODE_AF_PP,
            .Pull = GPIO_NOPULL,
            .Speed = GPIO_SPEED_FREQ_LOW,
            .Alternate = GPIO_AF3_MCO2,
        };
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }

    { // ADC_INT
        static constexpr GPIO_InitTypeDef GPIO_InitStruct {
            .Pin = GPIO_PIN_4,
            .Mode = GPIO_MODE_IT_FALLING,
            .Pull = GPIO_NOPULL,
            .Speed = GPIO_SPEED_FREQ_LOW,
            .Alternate = 0,
        };
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    }

    hal::spi::spi1_init();

    HAL_NVIC_SetPriority(DMAMUX1_DMA1_CH4_5_6_7_IRQn, ISR_PRIORITY_DEFAULT, 0);
    HAL_NVIC_EnableIRQ(DMAMUX1_DMA1_CH4_5_6_7_IRQn);

    HAL_NVIC_SetPriority(EXTI4_15_IRQn, ISR_PRIORITY_DEFAULT, 0);
    HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);

    HAL_NVIC_SetPriority(EXTI0_1_IRQn, ISR_PRIORITY_DEFAULT, 0);
    HAL_NVIC_EnableIRQ(EXTI0_1_IRQn);
}

void hal::init_i2c() {
    using namespace peripherals;

    // I2C runs its transfers over DMA (see hal_i2c.cpp). init_i2c() runs before
    // init_adc(), so enable the DMA1 clock here too (idempotent).
    __HAL_RCC_DMA1_CLK_ENABLE();

    static constexpr RCC_PeriphCLKInitTypeDef PeriphClkInit {
        .PeriphClockSelection = RCC_PERIPHCLK_I2C1,
        .HSIKerClockDivider = RCC_HSIKER_DIV1,
        .Usart1ClockSelection = 0,
        .I2c1ClockSelection = RCC_I2C1CLKSOURCE_PCLK1,
        .I2s1ClockSelection = 0,
        .Fdcan1ClockSelection = 0,
        .AdcClockSelection = 0,
        .RTCClockSelection = 0,
    };
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
        bsod_system();
    }

    __HAL_RCC_I2C1_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**I2C1 GPIO Configuration
    PB7     ------> I2C1_SDA
    PB8     ------> I2C1_SCL
    */
    static constexpr GPIO_InitTypeDef GPIO_InitStruct {
        .Pin = GPIO_PIN_7 | GPIO_PIN_8,
        .Mode = GPIO_MODE_AF_OD,
        .Pull = GPIO_NOPULL,
        .Speed = GPIO_SPEED_FREQ_LOW,
        .Alternate = GPIO_AF6_I2C1,
    };
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    hi2c1.Instance = I2C1;
    hi2c1.Init.Timing = 0x0090194B;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
        bsod_system();
    }

    if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK) {
        bsod_system();
    }

    if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK) {
        bsod_system();
    }

    // DMA1_Channel2 = I2C1_TX, DMA1_Channel3 = I2C1_RX. These share the
    // dedicated DMA1_Channel2_3_IRQn vector (independent of the SPI DMA ISR).
    hdma_i2c1_tx.Instance = DMA1_Channel2;
    hdma_i2c1_tx.Init.Request = DMA_REQUEST_I2C1_TX;
    hdma_i2c1_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_i2c1_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_i2c1_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_i2c1_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_i2c1_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_i2c1_tx.Init.Mode = DMA_NORMAL;
    hdma_i2c1_tx.Init.Priority = DMA_PRIORITY_LOW;
    if (HAL_DMA_Init(&hdma_i2c1_tx) != HAL_OK) {
        bsod_system();
    }
    __HAL_LINKDMA(&hi2c1, hdmatx, hdma_i2c1_tx);

    hdma_i2c1_rx.Instance = DMA1_Channel3;
    hdma_i2c1_rx.Init.Request = DMA_REQUEST_I2C1_RX;
    hdma_i2c1_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_i2c1_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_i2c1_rx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_i2c1_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_i2c1_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_i2c1_rx.Init.Mode = DMA_NORMAL;
    hdma_i2c1_rx.Init.Priority = DMA_PRIORITY_LOW;
    if (HAL_DMA_Init(&hdma_i2c1_rx) != HAL_OK) {
        bsod_system();
    }
    __HAL_LINKDMA(&hi2c1, hdmarx, hdma_i2c1_rx);

    HAL_NVIC_SetPriority(I2C1_IRQn, ISR_PRIORITY_DEFAULT, 0);
    HAL_NVIC_EnableIRQ(I2C1_IRQn);

    HAL_NVIC_SetPriority(DMA1_Channel2_3_IRQn, ISR_PRIORITY_DEFAULT, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel2_3_IRQn);
}

void hal::init_adc() {
    using namespace peripherals;

    __HAL_RCC_DMA1_CLK_ENABLE();

    static constexpr RCC_PeriphCLKInitTypeDef PeriphClkInit {
        .PeriphClockSelection = RCC_PERIPHCLK_ADC,
        .HSIKerClockDivider = RCC_HSIKER_DIV1,
        .Usart1ClockSelection = 0,
        .I2c1ClockSelection = 0,
        .I2s1ClockSelection = 0,
        .Fdcan1ClockSelection = 0,
        .AdcClockSelection = RCC_ADCCLKSOURCE_SYSCLK,
        .RTCClockSelection = 0,
    };
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
        bsod_system();
    }

    /* Peripheral clock enable */
    __HAL_RCC_ADC_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**ADC1 GPIO Configuration
    PA1     ------> Board temp
    PA4     ------> Heater feadback
    PB1     ------> Input volatage
    PB2     ------> Heater current
    */
    {
        static constexpr GPIO_InitTypeDef GPIO_InitStruct {
            .Pin = GPIO_PIN_1 | GPIO_PIN_4,
            .Mode = GPIO_MODE_ANALOG,
            .Pull = GPIO_NOPULL,
            .Speed = GPIO_SPEED_FREQ_LOW,
            .Alternate = 0,
        };
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }

    {
        static constexpr GPIO_InitTypeDef GPIO_InitStruct {
            .Pin = GPIO_PIN_1 | GPIO_PIN_2,
            .Mode = GPIO_MODE_ANALOG,
            .Pull = GPIO_NOPULL,
            .Speed = GPIO_SPEED_FREQ_LOW,
            .Alternate = 0,
        };
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    }

    hadc1.Instance = ADC1;
    hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
    hadc1.Init.Resolution = ADC_RESOLUTION_12B;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.ScanConvMode = ADC_SCAN_SEQ_FIXED;
    hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    hadc1.Init.LowPowerAutoWait = DISABLE;
    hadc1.Init.LowPowerAutoPowerOff = DISABLE;
    hadc1.Init.ContinuousConvMode = ENABLE;
    hadc1.Init.NbrOfConversion = 1;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.DMAContinuousRequests = ENABLE;
    hadc1.Init.Overrun = ADC_OVR_DATA_PRESERVED;
    hadc1.Init.SamplingTimeCommon1 = ADC_SAMPLETIME_3CYCLES_5;
    hadc1.Init.OversamplingMode = DISABLE;
    hadc1.Init.Oversampling.Ratio = ADC_OVERSAMPLING_RATIO_2;
    hadc1.Init.Oversampling.RightBitShift = ADC_RIGHTBITSHIFT_NONE;
    hadc1.Init.Oversampling.TriggeredMode = ADC_TRIGGEREDMODE_SINGLE_TRIGGER;
    hadc1.Init.TriggerFrequencyMode = ADC_TRIGGER_FREQ_HIGH;

    hdma_adc1.Instance = DMA1_Channel1;
    hdma_adc1.Init.Request = DMA_REQUEST_ADC1;
    hdma_adc1.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_adc1.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_adc1.Init.MemInc = DMA_MINC_ENABLE;
    hdma_adc1.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma_adc1.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    hdma_adc1.Init.Mode = DMA_CIRCULAR;
    hdma_adc1.Init.Priority = DMA_PRIORITY_LOW;
    if (HAL_DMA_Init(&hdma_adc1) != HAL_OK) {
        bsod_system();
    }

    __HAL_LINKDMA(&hadc1, DMA_Handle, hdma_adc1);

    static constexpr ADC_AnalogWDGConfTypeDef AnalogWDGConfig {
        .WatchdogNumber = ADC_ANALOGWATCHDOG_1,
        .WatchdogMode = ADC_ANALOGWATCHDOG_SINGLE_REG,
        .Channel = ADC_CHANNEL_19,
        .ITMode = ENABLE,
        .HighThreshold = 4095,
        .LowThreshold = 0,
    };
    if (HAL_ADC_AnalogWDGConfig(&hadc1, &AnalogWDGConfig) != HAL_OK) {
        bsod_system();
    }

    HAL_NVIC_SetPriority(ADC1_IRQn, ISR_PRIORITY_DEFAULT, 0);
    HAL_NVIC_EnableIRQ(ADC1_IRQn);

    HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, ISR_PRIORITY_DEFAULT, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);

    static_assert(alignof(adc::impl::buffer) == std::alignment_of_v<uint32_t>);
    config_adc_default();
}

// Re-configure ADC for default operation
void hal::config_adc_default() {
    using namespace peripherals;
    current_adc_config = ADC_Config::DEFAULT;

    if (HAL_ADC_Stop_DMA(&hadc1) != HAL_OK) {
        bsod_system();
    }

    hadc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV16;
    hadc1.Init.DMAContinuousRequests = ENABLE;
    if (HAL_ADC_Init(&hadc1) != HAL_OK) {
        bsod_system();
    }

    {
        static constexpr ADC_ChannelConfTypeDef sConfig {
            .Channel = ADC_CHANNEL_1,
            .Rank = ADC_RANK_CHANNEL_NUMBER,
            .SamplingTime = ADC_SAMPLETIME_160CYCLES_5,
        };
        if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
            bsod_system();
        }
    }

    {
        static constexpr ADC_ChannelConfTypeDef sConfig {
            .Channel = ADC_CHANNEL_4,
            .Rank = ADC_RANK_CHANNEL_NUMBER, // TODO - set to rank none
            .SamplingTime = ADC_SAMPLETIME_160CYCLES_5,
        };
        if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
            bsod_system();
        }
    }

    {
        static constexpr ADC_ChannelConfTypeDef sConfig {
            .Channel = ADC_CHANNEL_TEMPSENSOR,
            .Rank = ADC_RANK_CHANNEL_NUMBER,
            .SamplingTime = ADC_SAMPLETIME_160CYCLES_5,
        };
        if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
            bsod_system();
        }
    }

    {
        static constexpr ADC_ChannelConfTypeDef sConfig {
            .Channel = ADC_CHANNEL_VREFINT,
            .Rank = ADC_RANK_CHANNEL_NUMBER,
            .SamplingTime = ADC_SAMPLETIME_160CYCLES_5,
        };
        if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
            bsod_system();
        }
    }

    {
        static constexpr ADC_ChannelConfTypeDef sConfig {
            .Channel = ADC_CHANNEL_18,
            .Rank = ADC_RANK_CHANNEL_NUMBER,
            .SamplingTime = ADC_SAMPLETIME_160CYCLES_5,
        };
        if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
            bsod_system();
        }
    }

    {
        static constexpr ADC_ChannelConfTypeDef sConfig {
            .Channel = ADC_CHANNEL_19,
            .Rank = ADC_RANK_CHANNEL_NUMBER,
            .SamplingTime = ADC_SAMPLETIME_160CYCLES_5,
        };
        if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
            bsod_system();
        }
    }

    if (HAL_ADC_Start_DMA(&hadc1, reinterpret_cast<uint32_t *>(adc::impl::buffer.data()), adc::impl::buffer.size()) != HAL_OK) {
        bsod_system();
    }
}

// Re-configure ADC for induction heater feedback burst read.
// Very fast sampling of feedback waveform is required, so all other chanels are disabled
void hal::config_adc_induction_heater() {
    using namespace peripherals;
    if (HAL_ADC_Stop_DMA(&hadc1) != HAL_OK) {
        bsod_system();
    }
    current_adc_config = ADC_Config::INDUCTION_HEATER;

    hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV1;
    hadc1.Init.DMAContinuousRequests = DISABLE;
    if (HAL_ADC_Init(&hadc1) != HAL_OK) {
        bsod_system();
    }

    {
        static constexpr ADC_ChannelConfTypeDef sConfig {
            .Channel = ADC_CHANNEL_1,
            .Rank = ADC_RANK_NONE,
            .SamplingTime = ADC_SAMPLETIME_3CYCLES_5,
        };
        if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
            bsod_system();
        }
    }

    {
        static constexpr ADC_ChannelConfTypeDef sConfig {
            .Channel = ADC_CHANNEL_4,
            .Rank = ADC_RANK_CHANNEL_NUMBER,
            .SamplingTime = ADC_SAMPLETIME_3CYCLES_5,
        };
        if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
            bsod_system();
        }
    }

    {
        static constexpr ADC_ChannelConfTypeDef sConfig {
            .Channel = ADC_CHANNEL_TEMPSENSOR,
            .Rank = ADC_RANK_NONE,
            .SamplingTime = ADC_SAMPLETIME_3CYCLES_5,
        };
        if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
            bsod_system();
        }
    }

    {
        static constexpr ADC_ChannelConfTypeDef sConfig {
            .Channel = ADC_CHANNEL_VREFINT,
            .Rank = ADC_RANK_NONE,
            .SamplingTime = ADC_SAMPLETIME_3CYCLES_5,
        };
        if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
            bsod_system();
        }
    }

    {
        static constexpr ADC_ChannelConfTypeDef sConfig {
            .Channel = ADC_CHANNEL_18,
            .Rank = ADC_RANK_NONE,
            .SamplingTime = ADC_SAMPLETIME_3CYCLES_5,
        };
        if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
            bsod_system();
        }
    }

    {
        static constexpr ADC_ChannelConfTypeDef sConfig {
            .Channel = ADC_CHANNEL_19,
            .Rank = ADC_RANK_NONE,
            .SamplingTime = ADC_SAMPLETIME_3CYCLES_5,
        };
        if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
            bsod_system();
        }
    }
}

void hal::init_tim1() {
    using namespace peripherals;

    __HAL_RCC_TIM1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**TIM1 GPIO Configuration
    PA8     ------> TIM1_CH1 - Heatbreak Fan PWM
    PA9     ------> TIM1_CH2 - Print Fan PWM
    */
    static constexpr GPIO_InitTypeDef GPIO_InitStruct {
        .Pin = GPIO_PIN_8 | GPIO_PIN_9,
        .Mode = GPIO_MODE_AF_PP,
        .Pull = GPIO_PULLDOWN,
        .Speed = GPIO_SPEED_FREQ_HIGH,
        .Alternate = GPIO_AF2_TIM1,
    };
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    static constexpr uint32_t desired_frequency = 1000; // 1 kHz
    static constexpr uint32_t clock_resolution = 256;
    static constexpr uint32_t prescaler = (system_clock / (desired_frequency * clock_resolution)) - 1;

    htim1.Instance = TIM1;
    htim1.Init.Prescaler = prescaler;
    htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim1.Init.Period = clock_resolution - 1;
    htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim1.Init.RepetitionCounter = 0;
    htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_PWM_Init(&htim1) != HAL_OK) {
        bsod_system();
    }

    static constexpr TIM_MasterConfigTypeDef sMasterConfig {
        .MasterOutputTrigger = TIM_TRGO_RESET,
        .MasterOutputTrigger2 = TIM_TRGO2_RESET,
        .MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE,
    };
    if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK) {
        bsod_system();
    }

    static constexpr TIM_OC_InitTypeDef sConfigOC {
        .OCMode = TIM_OCMODE_PWM1,
        .Pulse = clock_resolution - 1,
        .OCPolarity = TIM_OCPOLARITY_HIGH,
        .OCNPolarity = TIM_OCNPOLARITY_HIGH,
        .OCFastMode = TIM_OCFAST_DISABLE,
        .OCIdleState = TIM_OCIDLESTATE_RESET,
        .OCNIdleState = TIM_OCNIDLESTATE_RESET,
    };
    if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK) {
        bsod_system();
    }

    if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_2) != HAL_OK) {
        bsod_system();
    }

    // TODO: Configure break on Cortex Lock and set everything high
    static constexpr TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig {
        .OffStateRunMode = TIM_OSSR_DISABLE,
        .OffStateIDLEMode = TIM_OSSI_DISABLE,
        .LockLevel = TIM_LOCKLEVEL_OFF,
        .DeadTime = 0,
        .BreakState = TIM_BREAK_DISABLE,
        .BreakPolarity = TIM_BREAKPOLARITY_HIGH,
        .BreakFilter = 0,
        .BreakAFMode = TIM_BREAK_AFMODE_INPUT,
        .Break2State = TIM_BREAK_DISABLE,
        .Break2Polarity = TIM_BREAKPOLARITY_HIGH,
        .Break2Filter = 0,
        .Break2AFMode = TIM_BREAK_AFMODE_INPUT,
        .AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE,
    };

    if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK) {
        bsod_system();
    }

    if (HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1) != HAL_OK) {
        bsod_system();
    }

    if (HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2) != HAL_OK) {
        bsod_system();
    }
}

void hal::init_tim3() {
    using namespace peripherals;

    __HAL_RCC_TIM3_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**TIM3 GPIO Configuration
    PB5     ------> TIM3_CH2 - Heatsink tacho
    PB6     ------> TIM3_CH1 - Print fan tacho
    */
    {
        static constexpr GPIO_InitTypeDef GPIO_InitStruct {
            .Pin = GPIO_PIN_6,
            .Mode = GPIO_MODE_AF_PP,
            .Pull = GPIO_NOPULL,
            .Speed = GPIO_SPEED_FREQ_LOW,
            .Alternate = GPIO_AF12_TIM3,
        };
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    }

    {
        static constexpr GPIO_InitTypeDef GPIO_InitStruct {
            .Pin = GPIO_PIN_5,
            .Mode = GPIO_MODE_AF_PP,
            .Pull = GPIO_NOPULL,
            .Speed = GPIO_SPEED_FREQ_LOW,
            .Alternate = GPIO_AF1_TIM3,
        };
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    }

    static constexpr uint32_t desired_frequency = 2; // 2 Hz
    static constexpr uint32_t clock_resolution = 50'000;
    static constexpr uint32_t prescaler = (system_clock / (desired_frequency * clock_resolution)) - 1;
    htim3.Instance = TIM3;
    htim3.Init.Prescaler = prescaler;
    htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim3.Init.Period = clock_resolution - 1;
    htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_IC_Init(&htim3) != HAL_OK) {
        bsod_system();
    }

    static constexpr TIM_MasterConfigTypeDef sMasterConfig {
        .MasterOutputTrigger = TIM_TRGO_RESET,
        .MasterOutputTrigger2 = TIM_TRGO2_RESET,
        .MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE,
    };
    if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK) {
        bsod_system();
    }

    static constexpr TIM_IC_InitTypeDef sConfigIC {
        .ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING,
        .ICSelection = TIM_ICSELECTION_DIRECTTI,
        .ICPrescaler = TIM_ICPSC_DIV2,
        .ICFilter = 15, // Maximum filtering to reject noise during PWM off periods
    };
    if (HAL_TIM_IC_ConfigChannel(&htim3, &sConfigIC, TIM_CHANNEL_1) != HAL_OK) {
        bsod_system();
    }

    if (HAL_TIM_IC_ConfigChannel(&htim3, &sConfigIC, TIM_CHANNEL_2) != HAL_OK) {
        bsod_system();
    }

    // Enable TIM3 interrupt
    HAL_NVIC_SetPriority(TIM3_IRQn, ISR_PRIORITY_DEFAULT, 0);
    HAL_NVIC_EnableIRQ(TIM3_IRQn);

    // Start input capture with interrupts for fan RPM measurement
    if (HAL_TIM_IC_Start_IT(&htim3, TIM_CHANNEL_1) != HAL_OK) {
        bsod_system();
    }
    if (HAL_TIM_IC_Start_IT(&htim3, TIM_CHANNEL_2) != HAL_OK) {
        bsod_system();
    }

    // Start base timer with update interrupt for timeout detection
    if (HAL_TIM_Base_Start_IT(&htim3) != HAL_OK) {
        bsod_system();
    }
}

void hal::init_tim16() {
    using namespace peripherals;

    __HAL_RCC_TIM16_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**TIM16 GPIO Configuration
    PA6     ------> TIM16_CH1 - Heater PWM
    */
    LL_TIM_InitTypeDef TIM_InitStruct = {};
    LL_TIM_OC_InitTypeDef TIM_OC_InitStruct = {};
    LL_TIM_BDTR_InitTypeDef TIM_BDTRInitStruct = {};

    TIM_InitStruct.Prescaler = 0;
    TIM_InitStruct.CounterMode = LL_TIM_COUNTERMODE_UP;
    TIM_InitStruct.Autoreload = 65535;
    TIM_InitStruct.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
    TIM_InitStruct.RepetitionCounter = 0;
    LL_TIM_Init(TIM16, &TIM_InitStruct);
    LL_TIM_EnableARRPreload(TIM16);
    LL_TIM_OC_EnablePreload(TIM16, LL_TIM_CHANNEL_CH1);

    TIM_OC_InitStruct.OCMode = LL_TIM_OCMODE_PWM1;
    TIM_OC_InitStruct.OCState = LL_TIM_OCSTATE_ENABLE;
    TIM_OC_InitStruct.OCNState = LL_TIM_OCSTATE_DISABLE;
    TIM_OC_InitStruct.CompareValue = 1;
    TIM_OC_InitStruct.OCPolarity = LL_TIM_OCPOLARITY_LOW;
    TIM_OC_InitStruct.OCNPolarity = LL_TIM_OCPOLARITY_HIGH;
    TIM_OC_InitStruct.OCIdleState = LL_TIM_OCIDLESTATE_LOW;
    TIM_OC_InitStruct.OCNIdleState = LL_TIM_OCIDLESTATE_LOW;
    LL_TIM_OC_Init(TIM16, LL_TIM_CHANNEL_CH1, &TIM_OC_InitStruct);
    LL_TIM_OC_DisableFast(TIM16, LL_TIM_CHANNEL_CH1);
    LL_TIM_SetOnePulseMode(TIM16, LL_TIM_ONEPULSEMODE_SINGLE);

    TIM_BDTRInitStruct.OSSRState = LL_TIM_OSSR_DISABLE;
    TIM_BDTRInitStruct.OSSIState = LL_TIM_OSSI_DISABLE;
    TIM_BDTRInitStruct.LockLevel = LL_TIM_LOCKLEVEL_OFF;
    TIM_BDTRInitStruct.DeadTime = 0;
    TIM_BDTRInitStruct.BreakState = LL_TIM_BREAK_DISABLE;
    TIM_BDTRInitStruct.BreakPolarity = LL_TIM_BREAK_POLARITY_HIGH;
    TIM_BDTRInitStruct.BreakFilter = LL_TIM_BREAK_FILTER_FDIV1;
    TIM_BDTRInitStruct.AutomaticOutput = LL_TIM_AUTOMATICOUTPUT_DISABLE;
    LL_TIM_BDTR_Init(TIM16, &TIM_BDTRInitStruct);

    LL_GPIO_InitTypeDef GPIO_InitStruct = {};
    GPIO_InitStruct.Pin = LL_GPIO_PIN_6;
    GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
    GPIO_InitStruct.Alternate = LL_GPIO_AF_5;
    LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    HAL_NVIC_SetPriority(TIM16_IRQn, ISR_PRIORITY_DEFAULT, 0);
    HAL_NVIC_EnableIRQ(TIM16_IRQn);

    TIM16->EGR = TIM_EGR_UG; // Forcing update to ensure output low
    LL_TIM_EnableAllOutputs(TIM16);
}

extern "C" void hal_panic() {
    hal::panic(indx_head::errors::FaultStatusMask::assert_failed);
}

extern "C" void NMI_Handler() {
    hal::panic(indx_head::errors::FaultStatusMask::hard_fault);
}

extern "C" void HardFault_Handler() {
    hal::panic(indx_head::errors::FaultStatusMask::hard_fault);
}

extern "C" void MemManage_Handler() {
    hal::panic(indx_head::errors::FaultStatusMask::hard_fault);
}

extern "C" void BusFault_Handler() {
    hal::panic(indx_head::errors::FaultStatusMask::hard_fault);
}

extern "C" void UsageFault_Handler() {
    hal::panic(indx_head::errors::FaultStatusMask::hard_fault);
}

// SysTick_Handler defined by FreeRTOS, defining FreeRTOS hook to increment HAL tick if needed
extern "C" void vApplicationTickHook(void) {
    HAL_IncTick();
    watchdog::manager().tick();
}

extern "C" void EXTI0_1_IRQHandler(void) {
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
}

extern "C" void EXTI4_15_IRQHandler(void) {
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_4);
}

extern "C" void HAL_GPIO_EXTI_Rising_Callback([[maybe_unused]] uint16_t gpio_pin) {
    debug_assert(gpio_pin == GPIO_PIN_0);
    spi_task::notify_accel_data_ready();
}

extern "C" void HAL_GPIO_EXTI_Falling_Callback([[maybe_unused]] uint16_t gpio_pin) {
    debug_assert(gpio_pin == GPIO_PIN_4);
    spi_task::notify_loadcell_data_ready();
}

extern "C" void DMA1_Channel1_IRQHandler(void) {
    using namespace hal::peripherals;
    HAL_DMA_IRQHandler(&hdma_adc1);
}

extern "C" void DMA1_Channel2_3_IRQHandler(void) {
    using namespace hal::peripherals;
    HAL_DMA_IRQHandler(&hdma_i2c1_tx);
    HAL_DMA_IRQHandler(&hdma_i2c1_rx);
}

extern "C" void ADC1_IRQHandler(void) {
    using namespace hal::peripherals;
    HAL_ADC_IRQHandler(&hadc1);
}

extern "C" void HAL_ADC_ConvCpltCallback([[maybe_unused]] ADC_HandleTypeDef *hadc) {
    using namespace hal;
    if (current_adc_config == ADC_Config::INDUCTION_HEATER) {
        peripherals::adc_semaphore.release_from_isr();
    }
}

extern "C" void HAL_ADC_ErrorCallback(ADC_HandleTypeDef *hadc) {
    // Root cause of the random "INDX stops answering Modbus" hang:
    // The induction-heater ADC mode switching (config_adc_*, repeated
    // HAL_ADC_Start_DMA/Stop_DMA in InductionHeater::measure) can occasionally
    // leave the ADC converting while DMA is not draining -> overrun (OVR). The HAL
    // clears the OVR flag and calls this callback but does NOT stop the
    // conversions, so OVR is set again immediately and ADC1_IRQ re-fires forever.
    // That ISR storm (priority 2) starves the lowest-priority SysTick and PendSV,
    // freezing the FreeRTOS scheduler: the modbus task never runs again and the
    // head stays mute until XBuddy hard-resets it.
    //
    // Break the storm at the source: disable the overrun interrupt. It is
    // re-enabled cleanly by the next HAL_ADC_Start_DMA (config_adc_* runs every
    // heater cycle), so the ADC self-recovers without a reset.
    if (hadc == &hal::peripherals::hadc1 && (hadc->ErrorCode & HAL_ADC_ERROR_OVR) != 0U) {
        __HAL_ADC_DISABLE_IT(hadc, ADC_IT_OVR);
    }
}

extern "C" void I2C1_IRQHandler(void) {
    using namespace hal::peripherals;
    if (hi2c1.Instance->ISR & (I2C_FLAG_BERR | I2C_FLAG_ARLO | I2C_FLAG_OVR)) {
        HAL_I2C_ER_IRQHandler(&hi2c1);
    } else {
        HAL_I2C_EV_IRQHandler(&hi2c1);
    }
}

extern "C" void DMAMUX1_DMA1_CH4_5_6_7_IRQHandler(void) {
    hal::spi::dma_isr();
}

extern "C" void TIM3_IRQHandler(void) {
    using namespace hal::peripherals;
    HAL_TIM_IRQHandler(&htim3);
}

extern "C" void TIM16_IRQHandler(void) {
    inductionHeater.ramp_isr();
}
