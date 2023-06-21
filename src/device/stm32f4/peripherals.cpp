#include <device/board.h>
#include <device/peripherals.h>
#include <device/mcu.h>
#include <atomic>
#include "Pin.hpp"
#include "hwio_pindef.h"
#include "main.h"
#include "adc.hpp"
#include "timer_defaults.h"
#include "PCA9557.hpp"
#include "log.h"

//
// I2C
//

I2C_HandleTypeDef hi2c1;
I2C_HandleTypeDef hi2c2;
I2C_HandleTypeDef hi2c3;

//
// SPI
//

SPI_HandleTypeDef hspi2;
DMA_HandleTypeDef hdma_spi2_rx;
DMA_HandleTypeDef hdma_spi2_tx;
SPI_HandleTypeDef hspi3;
DMA_HandleTypeDef hdma_spi3_rx;
DMA_HandleTypeDef hdma_spi3_tx;
SPI_HandleTypeDef hspi4;
DMA_HandleTypeDef hdma_spi4_tx;
SPI_HandleTypeDef hspi5;
DMA_HandleTypeDef hdma_spi5_tx;
DMA_HandleTypeDef hdma_spi5_rx;
SPI_HandleTypeDef hspi6;
DMA_HandleTypeDef hdma_spi6_tx;
DMA_HandleTypeDef hdma_spi6_rx;

//
// UART
//

UART_HandleTypeDef huart1;
DMA_HandleTypeDef hdma_usart1_rx;
UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart2_rx;
UART_HandleTypeDef huart3;
DMA_HandleTypeDef hdma_usart3_rx;
DMA_HandleTypeDef hdma_usart3_tx;
UART_HandleTypeDef huart6;
DMA_HandleTypeDef hdma_usart6_rx;
DMA_HandleTypeDef hdma_usart6_tx;
UART_HandleTypeDef huart8;
DMA_HandleTypeDef hdma_uart8_rx;
DMA_HandleTypeDef hdma_uart8_tx;

//
// ADCs
//

ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;
ADC_HandleTypeDef hadc2;
DMA_HandleTypeDef hdma_adc2;
ADC_HandleTypeDef hadc3;
DMA_HandleTypeDef hdma_adc3;

//
// Timers
//

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim14;

//
// Other
//

RTC_HandleTypeDef hrtc;
RNG_HandleTypeDef hrng;

#if BOARD_IS_XLBUDDY
buddy::hw::PCA9557 io_expander1(I2C_HANDLE_FOR(io_extender), 0x1);
#endif

//
// Initialization
//

void hw_rtc_init() {
    hrtc.Instance = RTC;
    hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
    hrtc.Init.AsynchPrediv = 127;
    hrtc.Init.SynchPrediv = 255;
    hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
    hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
    if (HAL_RTC_Init(&hrtc) != HAL_OK) {
        Error_Handler();
    }
    HAL_RTC_DeactivateAlarm(&hrtc, RTC_ALARM_A);
    HAL_RTC_DeactivateAlarm(&hrtc, RTC_ALARM_B);
    HAL_RTCEx_DeactivateCalibrationOutPut(&hrtc);
    HAL_RTCEx_DeactivateTamper(&hrtc, RTC_TAFCR_TAMP1E);
    HAL_RTCEx_DeactivateTimeStamp(&hrtc);
}

void hw_rng_init() {
    hrng.Instance = RNG;
    if (HAL_RNG_Init(&hrng) != HAL_OK) {
        Error_Handler();
    }
}

void hw_gpio_init() {
    GPIO_InitTypeDef GPIO_InitStruct {};

    // GPIO Ports Clock Enable
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();

    // Configure GPIO pins : USB_OVERC_Pin ESP_GPIO0_Pin BED_MON_Pin WP1_Pin
    GPIO_InitStruct.Pin = USB_OVERC_Pin | ESP_GPIO0_Pin | BED_MON_Pin | WP1_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    // NOTE: Configuring GPIO causes a short drop of pin output to low. This is
    //       avoided by first setting the pin and then initilizing the GPIO. In case
    //       this does not work we first initilize ESP GPIO0 to avoid reset low
    //       followed by ESP GPIO low as this sequence can switch esp to boot mode */

    // Configure ESP GPIO0 (PROG, High for ESP module boot from Flash)
    GPIO_InitStruct.Pin =
#if (BOARD_IS_XBUDDY || BOARD_IS_XLBUDDY)
        GPIO_PIN_15
#else
        GPIO_PIN_6
#endif
        ;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_WritePin(GPIOE,
#if (BOARD_IS_XBUDDY || BOARD_IS_XLBUDDY)
        GPIO_PIN_15
#else
        GPIO_PIN_6
#endif
        ,
        GPIO_PIN_SET);
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    // Configure GPIO pins : ESP_RST_Pin
    GPIO_InitStruct.Pin = ESP_RST_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_WritePin(GPIOC, ESP_RST_Pin, GPIO_PIN_SET);
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    // Configure GPIO pins : WP2_Pin
    GPIO_InitStruct.Pin = WP2_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    PIN_TABLE(CONFIGURE_PINS);
#if defined(EXTENDER_PIN_TABLE)
    EXTENDER_PIN_TABLE(CONFIGURE_PINS);
#endif

    buddy::hw::hwio_configure_board_revision_changed_pins();
}

void hw_dma_init() {
    // DMA controller clock enable
    __HAL_RCC_DMA1_CLK_ENABLE();
    __HAL_RCC_DMA2_CLK_ENABLE();

#if (!PRINTER_IS_PRUSA_MINI)
    // DMA1_Stream3_IRQn interrupt configuration
    HAL_NVIC_SetPriority(DMA1_Stream3_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream3_IRQn);
#endif

#if (BOARD_IS_XBUDDY || BOARD_IS_XLBUDDY)
    // DMA1_Stream0_IRQn interrupt configuration
    HAL_NVIC_SetPriority(DMA1_Stream0_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream0_IRQn);
    // DMA1_Stream2_IRQn interrupt configuration
    HAL_NVIC_SetPriority(DMA1_Stream2_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream2_IRQn);
    // DMA1_Stream6_IRQn interrupt configuration
    HAL_NVIC_SetPriority(DMA1_Stream6_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream6_IRQn);
    // DMA2_Stream3_IRQn interrupt configuration
    HAL_NVIC_SetPriority(DMA2_Stream3_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY, 0);
    HAL_NVIC_EnableIRQ(DMA2_Stream3_IRQn);
    // DMA2_Stream4_IRQn interrupt configuration
    HAL_NVIC_SetPriority(DMA2_Stream4_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY, 0);
    HAL_NVIC_EnableIRQ(DMA2_Stream4_IRQn);
    // DMA2_Stream5_IRQn interrupt configuration
    HAL_NVIC_SetPriority(DMA2_Stream5_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY, 0);
    HAL_NVIC_EnableIRQ(DMA2_Stream5_IRQn);
    // DMA2_Stream6_IRQn interrupt configuration
    HAL_NVIC_SetPriority(DMA2_Stream6_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY, 0);
    HAL_NVIC_EnableIRQ(DMA2_Stream6_IRQn);
    // DMA2_Stream7_IRQn interrupt configuration
    HAL_NVIC_SetPriority(DMA2_Stream7_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY, 0);
    HAL_NVIC_EnableIRQ(DMA2_Stream7_IRQn);
#endif
    // DMA1_Stream0_IRQn interrupt configuration
    HAL_NVIC_SetPriority(DMA1_Stream0_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream0_IRQn);

#if BOARD_IS_XLBUDDY
    // DMA1_Stream1_IRQn interrupt configuration
    HAL_NVIC_SetPriority(DMA1_Stream1_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream1_IRQn);

    // DMA2_Stream0_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY, 0);
    HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);
#endif

    // DMA1_Stream4_IRQn interrupt configuration
    HAL_NVIC_SetPriority(DMA1_Stream4_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream4_IRQn);
    // DMA1_Stream5_IRQn interrupt configuration
    HAL_NVIC_SetPriority(DMA1_Stream5_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);
    // DMA1_Stream7_IRQn interrupt configuration
    HAL_NVIC_SetPriority(DMA1_Stream7_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream7_IRQn);
    // DMA2_Stream1_IRQn interrupt configuration
    HAL_NVIC_SetPriority(DMA2_Stream1_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY, 0);
    HAL_NVIC_EnableIRQ(DMA2_Stream1_IRQn);
    // DMA2_Stream2_IRQn interrupt configuration
    HAL_NVIC_SetPriority(DMA2_Stream2_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY, 0);
    HAL_NVIC_EnableIRQ(DMA2_Stream2_IRQn);
    // DMA2_Stream2_IRQn interrupt configuration
    HAL_NVIC_SetPriority(DMA2_Stream4_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY, 0);
    HAL_NVIC_EnableIRQ(DMA2_Stream4_IRQn);
}

void static config_adc(ADC_HandleTypeDef *hadc, ADC_TypeDef *ADC_NUM, uint32_t NbrOfConversion) {
    hadc->Instance = ADC_NUM;
    hadc->Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV8;
    hadc->Init.Resolution = ADC_RESOLUTION_12B;
    hadc->Init.ScanConvMode = ENABLE;
    hadc->Init.ContinuousConvMode = ENABLE;
    hadc->Init.DiscontinuousConvMode = DISABLE;
    hadc->Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc->Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc->Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc->Init.NbrOfConversion = NbrOfConversion;
    hadc->Init.DMAContinuousRequests = ENABLE;
    hadc->Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    if (HAL_ADC_Init(hadc) != HAL_OK) {
        Error_Handler();
    }
}

static void config_adc_ch(ADC_HandleTypeDef *hadc, uint32_t Channel, uint32_t Rank) {
    Rank++; // Channel rank starts at 1, but for array indexing, we need to start from 0.
    ADC_ChannelConfTypeDef sConfig = { Channel, Rank, ADC_SAMPLETIME_480CYCLES, 0 };
    if (HAL_ADC_ConfigChannel(hadc, &sConfig) != HAL_OK) {
        Error_Handler();
    }
}

void hw_adc1_init() {
    // Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
    config_adc(&hadc1, ADC1, AdcChannel::ADC1_CH_CNT);

    // Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
#if (BOARD_IS_BUDDY)
    config_adc_ch(&hadc1, ADC_CHANNEL_10, AdcChannel::hotend_T);
    config_adc_ch(&hadc1, ADC_CHANNEL_4, AdcChannel::heatbed_T);
    config_adc_ch(&hadc1, ADC_CHANNEL_5, AdcChannel::board_T);
    config_adc_ch(&hadc1, ADC_CHANNEL_6, AdcChannel::pinda_T);
    config_adc_ch(&hadc1, ADC_CHANNEL_3, AdcChannel::heatbed_U);
#elif (BOARD_IS_XBUDDY && BOARD_VER_EQUAL_TO(0, 3, 4))
    config_adc_ch(&hadc1, ADC_CHANNEL_10, AdcChannel::hotend_T);
    config_adc_ch(&hadc1, ADC_CHANNEL_4, AdcChannel::heatbed_T);
    config_adc_ch(&hadc1, ADC_CHANNEL_6, AdcChannel::heatbreak_T);
    config_adc_ch(&hadc1, ADC_CHANNEL_5, AdcChannel::hotend_U);
#elif (BOARD_IS_XBUDDY && PRINTER_IS_PRUSA_MK3_5)
    config_adc_ch(&hadc1, ADC_CHANNEL_10, AdcChannel::hotend_T);
    config_adc_ch(&hadc1, ADC_CHANNEL_4, AdcChannel::heatbed_T);
    config_adc_ch(&hadc1, ADC_CHANNEL_5, AdcChannel::heatbed_U);
    config_adc_ch(&hadc1, ADC_CHANNEL_3, AdcChannel::hotend_U);
#elif (BOARD_IS_XBUDDY)
    config_adc_ch(&hadc1, ADC_CHANNEL_10, AdcChannel::hotend_T);
    config_adc_ch(&hadc1, ADC_CHANNEL_4, AdcChannel::heatbed_T);
    config_adc_ch(&hadc1, ADC_CHANNEL_5, AdcChannel::heatbed_U);
    config_adc_ch(&hadc1, ADC_CHANNEL_6, AdcChannel::heatbreak_T);
    config_adc_ch(&hadc1, ADC_CHANNEL_3, AdcChannel::hotend_U);
#elif BOARD_IS_XLBUDDY
    config_adc_ch(&hadc1, ADC_CHANNEL_4, AdcChannel::dwarf_I);
    config_adc_ch(&hadc1, ADC_CHANNEL_5, AdcChannel::mux1_y);
    config_adc_ch(&hadc1, ADC_CHANNEL_8, AdcChannel::mux1_x);
#else
    #error Unknown board
#endif

    HAL_NVIC_DisableIRQ(DMA2_Stream4_IRQn); // Disable ADC DMA IRQ. This IRQ is not used. Save CPU usage.
}

#ifdef HAS_ADC3
void hw_adc3_init() {
    // Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
    config_adc(&hadc3, ADC3, AdcChannel::ADC3_CH_CNT);

    // Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
    #if BOARD_IS_XBUDDY
    config_adc_ch(&hadc3, ADC_CHANNEL_4, AdcChannel::MMU_I);
    config_adc_ch(&hadc3, ADC_CHANNEL_8, AdcChannel::board_T);
    config_adc_ch(&hadc3, ADC_CHANNEL_9, AdcChannel::hotend_I);
    config_adc_ch(&hadc3, ADC_CHANNEL_14, AdcChannel::board_I);
    config_adc_ch(&hadc3, ADC_CHANNEL_15, AdcChannel::case_T);
    #elif BOARD_IS_XLBUDDY
    config_adc_ch(&hadc3, ADC_CHANNEL_8, AdcChannel::board_T);
    config_adc_ch(&hadc3, ADC_CHANNEL_4, AdcChannel::mux2_y);
    config_adc_ch(&hadc3, ADC_CHANNEL_10, AdcChannel::mux2_x);

    #else
        #error Unknown board
    #endif

    HAL_NVIC_DisableIRQ(DMA2_Stream0_IRQn); // Disable ADC DMA IRQ. This IRQ is not used. Save CPU usage.
}
#endif

void hw_uart1_init() {
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart1) != HAL_OK) {
        Error_Handler();
    }
}

void hw_uart2_init() {
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_HalfDuplex_Init(&huart2) != HAL_OK) {
        Error_Handler();
    }
}

void hw_uart3_init() {
    huart3.Instance = USART3;
    huart3.Init.BaudRate = 230400;
    huart3.Init.WordLength = UART_WORDLENGTH_8B;
    huart3.Init.StopBits = UART_STOPBITS_1;
    huart3.Init.Parity = UART_PARITY_NONE;
    huart3.Init.Mode = UART_MODE_TX_RX;
    huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart3.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart3) != HAL_OK) {
        Error_Handler();
    }
}

void hw_uart6_init() {
    huart6.Instance = USART6;
    huart6.Init.BaudRate = 115200;
    huart6.Init.WordLength = UART_WORDLENGTH_8B;
    huart6.Init.StopBits = UART_STOPBITS_1;
    huart6.Init.Parity = UART_PARITY_NONE;
    huart6.Init.Mode = UART_MODE_TX_RX;
    huart6.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart6.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart6) != HAL_OK) {
        Error_Handler();
    }
}

#if MCU_IS_STM32F42X()
void hw_uart8_init() {
    huart8.Instance = UART8;
    huart8.Init.BaudRate = 4600000;
    huart8.Init.WordLength = UART_WORDLENGTH_8B;
    huart8.Init.StopBits = UART_STOPBITS_1;
    huart8.Init.Parity = UART_PARITY_NONE;
    huart8.Init.Mode = UART_MODE_TX_RX;
    huart8.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart8.Init.OverSampling = UART_OVERSAMPLING_8;
    if (HAL_UART_Init(&huart8) != HAL_OK) {
        Error_Handler();
    }
}
#endif

LOG_COMPONENT_DEF(I2C, LOG_SEVERITY_INFO);

static void wait_for_pin(int &workaround_step, GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, GPIO_PinState pin_state, uint32_t max_wait_us = 128) {
    volatile uint32_t start_us = ticks_us();
    while (HAL_GPIO_ReadPin(GPIOx, GPIO_Pin) != pin_state) {
        volatile uint32_t now = ticks_us();
        if (((now - start_us) > max_wait_us) && HAL_GPIO_ReadPin(GPIOx, GPIO_Pin) != pin_state) {
            log_error(I2C, "not operational, busy reset step %d", workaround_step++);
            return;
        }
    }
}
#if HAS_I2CN(1)
static void hw_i2c1_base_init();
#endif
#if HAS_I2CN(2)
static void hw_i2c2_base_init();
#endif
#if HAS_I2CN(3)
static void hw_i2c3_base_init();
#endif

/**
 * @brief I2C is busy and does not communicate
 * I have found similar issue in STM32F1 error datasheet (but not in STM32F4)
 * Description
 * The I2C analog filters embedded in the I2C I/Os may be tied to low level, whereas SCL and SDA lines are kept at
 * high level. This can occur after an MCU power-on reset, or during ESD stress. Consequently, the I2C BUSY flag
 * is set, and the I2C cannot enter master mode (START condition cannot be sent). The I2C BUSY flag cannot be
 * cleared by the SWRST control bit, nor by a peripheral or a system reset. BUSY bit is cleared under reset, but it
 * is set high again as soon as the reset is released, because the analog filter output is still at low level. This issue
 * occurs randomly.
 *
 * Note: Under the same conditions, the I2C analog filters may also provide a high level, whereas SCL and SDA lines are
 * kept to low level. This should not create issues as the filters output is correct after next SCL and SDA transition.
 *
 *
 * Workaround
 * The SCL and SDA analog filter output is updated after a transition occurs on the SCL and SDA line respectively.
 * The SCL and SDA transition can be forced by software configuring the I2C I/Os in output mode. Then, once the
 * analog filters are unlocked and output the SCL and SDA lines level, the BUSY flag can be reset with a software
 * reset, and the I2C can enter master mode. Therefore, the following sequence must be applied:
 */
static void i2c_busy_flag_error_workaround(I2C_HandleTypeDef *hi2c, GPIO_TypeDef *SDA_PORT, uint32_t SDA_PIN, GPIO_TypeDef *SCL_PORT, uint32_t SCL_PIN, void (*init_fn)()) {
    int workaround_step = 0;

    GPIO_InitTypeDef GPIO_InitStruct;

    // 1. Disable the I2C peripheral by clearing the PE bit in I2Cx_CR1 register.
    __HAL_I2C_DISABLE(hi2c);

    // 2. Configure the SCL and SDA I/Os as General Purpose Output Open-Drain, High level (Write 1 to GPIOx_ODR).
    GPIO_InitStruct.Pin = SDA_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FAST;
    HAL_GPIO_Init(SDA_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = SCL_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FAST;
    HAL_GPIO_Init(SCL_PORT, &GPIO_InitStruct);

    HAL_GPIO_WritePin(SDA_PORT, SDA_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(SCL_PORT, SCL_PIN, GPIO_PIN_SET);

    // 3. Check SCL and SDA High level in GPIOx_IDR.
    wait_for_pin(workaround_step, SDA_PORT, SDA_PIN, GPIO_PIN_SET);
    wait_for_pin(workaround_step, SCL_PORT, SCL_PIN, GPIO_PIN_SET);

    // 4. Configure the SDA I/O as General Purpose Output Open-Drain, Low level (Write 0 to GPIOx_ODR).
    GPIO_InitStruct.Pin = SDA_PIN;
    HAL_GPIO_Init(SDA_PORT, &GPIO_InitStruct);

    HAL_GPIO_TogglePin(SDA_PORT, SDA_PIN);

    // 5. Check SDA Low level in GPIOx_IDR.
    wait_for_pin(workaround_step, SDA_PORT, SDA_PIN, GPIO_PIN_RESET);

    // 6. Configure the SCL I/O as General Purpose Output Open-Drain, Low level (Write 0 to GPIOx_ODR).
    GPIO_InitStruct.Pin = SCL_PIN;
    HAL_GPIO_Init(SCL_PORT, &GPIO_InitStruct);

    HAL_GPIO_TogglePin(SCL_PORT, SCL_PIN);

    // 7. Check SCL Low level in GPIOx_IDR.
    wait_for_pin(workaround_step, SCL_PORT, SCL_PIN, GPIO_PIN_RESET);

    // 8. Configure the SCL I/O as General Purpose Output Open-Drain, High level (Write 1 to GPIOx_ODR).
    GPIO_InitStruct.Pin = SDA_PIN;
    HAL_GPIO_Init(SDA_PORT, &GPIO_InitStruct);

    HAL_GPIO_WritePin(SDA_PORT, SDA_PIN, GPIO_PIN_SET);

    // 9. Check SCL High level in GPIOx_IDR.
    wait_for_pin(workaround_step, SDA_PORT, SDA_PIN, GPIO_PIN_SET);

    // 10. Configure the SDA I/O as General Purpose Output Open-Drain , High level (Write 1 to GPIOx_ODR).
    GPIO_InitStruct.Pin = SCL_PIN;
    HAL_GPIO_Init(SCL_PORT, &GPIO_InitStruct);

    HAL_GPIO_WritePin(SCL_PORT, SCL_PIN, GPIO_PIN_SET);

    // 11. Check SDA High level in GPIOx_IDR.
    wait_for_pin(workaround_step, SCL_PORT, SCL_PIN, GPIO_PIN_SET);

    // 12. Configure the SCL and SDA I/Os as Alternate function Open-Drain.
    GPIO_InitStruct.Pin = SDA_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Alternate = 0x04; // 4 == I2C
    HAL_GPIO_Init(SDA_PORT, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = SCL_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Alternate = 0x04; // 4 == I2C
    HAL_GPIO_Init(SCL_PORT, &GPIO_InitStruct);

    // 13. Set SWRST bit in I2Cx_CR1 register.
    hi2c->Instance->CR1 |= I2C_CR1_SWRST;

    // 14. Clear SWRST bit in I2Cx_CR1 register.
    hi2c->Instance->CR1 ^= I2C_CR1_SWRST;

    // need extra step - call init fnc
    init_fn();

    // 15. Enable the I2C peripheral by setting the PE bit in I2Cx_CR1 register.
    // this step is done during I2C init function
    __HAL_I2C_ENABLE(hi2c);
}

/// call busy flag reset function
#define I2C_BUSY_FLAG_ERROR_WORKAROUND(i2c_num) i2c_busy_flag_error_workaround(&hi2c##i2c_num, \
    i2c##i2c_num##_SDA_PORT, i2c##i2c_num##_SDA_PIN,                                           \
    i2c##i2c_num##_SCL_PORT, i2c##i2c_num##_SCL_PIN, hw_i2c##i2c_num##_base_init)

static std::atomic<size_t> i2c1_busy_clear_count = 0;
static std::atomic<size_t> i2c2_busy_clear_count = 0;
static std::atomic<size_t> i2c3_busy_clear_count = 0;

size_t hw_i2c1_get_busy_clear_count() { return i2c1_busy_clear_count.load(); }
size_t hw_i2c2_get_busy_clear_count() { return i2c2_busy_clear_count.load(); }
size_t hw_i2c3_get_busy_clear_count() { return i2c3_busy_clear_count.load(); }

#if HAS_I2CN(1)
void hw_i2c1_base_init() {
    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = 400000;

    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
        Error_Handler();
    }

    #if defined(I2C_FLTR_ANOFF) && defined(I2C_FLTR_DNF)
    // Configure Analog filter
    if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK) {
        Error_Handler();
    }
    // Configure Digital filter
    if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK) {
        Error_Handler();
    }
    #endif
}

void hw_i2c1_init() {
    hw_i2c1_base_init();

    if (__HAL_I2C_GET_FLAG(&hi2c1, I2C_FLAG_BUSY) == SET) {
        I2C_BUSY_FLAG_ERROR_WORKAROUND(1);
        ++i2c1_busy_clear_count;
    }
}
#endif // HAS_I2CN(1)

#if HAS_I2CN(2)
static void hw_i2c2_base_init() {
    hi2c2.Instance = I2C2;
    hi2c2.Init.ClockSpeed = 100000;

    hi2c2.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c2.Init.OwnAddress1 = 0;
    hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c2.Init.OwnAddress2 = 0;
    hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&hi2c2) != HAL_OK) {
        Error_Handler();
    }

    #if defined(I2C_FLTR_ANOFF) && defined(I2C_FLTR_DNF)
    // Configure Analog filter
    if (HAL_I2CEx_ConfigAnalogFilter(&hi2c2, I2C_ANALOGFILTER_ENABLE) != HAL_OK) {
        Error_Handler();
    }
    // Configure Digital filter
    if (HAL_I2CEx_ConfigDigitalFilter(&hi2c2, 0) != HAL_OK) {
        Error_Handler();
    }
    #endif
}

void hw_i2c2_init() {
    hw_i2c2_base_init();

    if (__HAL_I2C_GET_FLAG(&hi2c2, I2C_FLAG_BUSY) == SET) {
        I2C_BUSY_FLAG_ERROR_WORKAROUND(2);
        ++i2c2_busy_clear_count;
    }
}
#endif // HAS_I2CN(2)

#if HAS_I2CN(3)
static void hw_i2c3_base_init() {
    hi2c3.Instance = I2C3;
    hi2c3.Init.ClockSpeed = 100000;
    hi2c3.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c3.Init.OwnAddress1 = 0;
    hi2c3.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c3.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c3.Init.OwnAddress2 = 0;
    hi2c3.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c3.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&hi2c3) != HAL_OK) {
        Error_Handler();
    }

    #if defined(I2C_FLTR_ANOFF) && defined(I2C_FLTR_DNF)
    // Configure Analogue filter
    if (HAL_I2CEx_ConfigAnalogFilter(&hi2c3, I2C_ANALOGFILTER_ENABLE) != HAL_OK) {
        Error_Handler();
    }
    // Configure Digital filter
    if (HAL_I2CEx_ConfigDigitalFilter(&hi2c3, 0) != HAL_OK) {
        Error_Handler();
    }
    #endif
}

void hw_i2c3_init() {
    hw_i2c3_base_init();

    if (__HAL_I2C_GET_FLAG(&hi2c3, I2C_FLAG_BUSY) == SET) {
        I2C_BUSY_FLAG_ERROR_WORKAROUND(3);
        ++i2c3_busy_clear_count;
    }
}
#endif // HAS_I2CN(3)

void hw_spi2_init() {
    hspi2.Instance = SPI2;
    hspi2.Init.Mode = SPI_MODE_MASTER;
    hspi2.Init.Direction = SPI_DIRECTION_2LINES;
    hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi2.Init.NSS = SPI_NSS_SOFT;
    hspi2.Init.BaudRatePrescaler =
#if spi_accelerometer == 2
        SPI_BAUDRATEPRESCALER_8
#elif spi_lcd == 2
        SPI_BAUDRATEPRESCALER_2
#endif
        ;
    hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi2.Init.CRCPolynomial = 10;
    if (HAL_SPI_Init(&hspi2) != HAL_OK) {
        Error_Handler();
    }
}

void hw_spi3_init() {
    hspi3.Instance = SPI3;
    hspi3.Init.Mode = SPI_MODE_MASTER;
    hspi3.Init.Direction = SPI_DIRECTION_2LINES;
    hspi3.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi3.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi3.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi3.Init.NSS = SPI_NSS_SOFT;
#if (BOARD_IS_BUDDY)
    hspi3.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
#else
    hspi3.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
#endif
    hspi3.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi3.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi3.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi3.Init.CRCPolynomial = 10;
    if (HAL_SPI_Init(&hspi3) != HAL_OK) {
        Error_Handler();
    }
}

#if MCU_IS_STM32F42X()
void hw_spi4_init() {
    // SPI 4 is used for side leds, but only on specific HW revisions
    hspi4.Instance = SPI4;
    hspi4.Init.Mode = SPI_MODE_MASTER;
    hspi4.Init.Direction = SPI_DIRECTION_2LINES;
    hspi4.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi4.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi4.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi4.Init.NSS = SPI_NSS_SOFT;
    hspi4.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
    hspi4.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi4.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi4.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi4.Init.CRCPolynomial = 10;
    if (HAL_SPI_Init(&hspi4) != HAL_OK) {
        Error_Handler();
    }
}

void hw_spi5_init() {
    hspi5.Instance = SPI5;
    hspi5.Init.Mode = SPI_MODE_MASTER;
    hspi5.Init.Direction = SPI_DIRECTION_2LINES;
    hspi5.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi5.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi5.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi5.Init.NSS = SPI_NSS_SOFT;
    hspi5.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
    hspi5.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi5.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi5.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi5.Init.CRCPolynomial = 10;
    if (HAL_SPI_Init(&hspi5) != HAL_OK) {
        Error_Handler();
    }
}

void hw_spi6_init() {
    hspi6.Instance = SPI6;
    hspi6.Init.Mode = SPI_MODE_MASTER;
    hspi6.Init.Direction = SPI_DIRECTION_2LINES;
    hspi6.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi6.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi6.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi6.Init.NSS = SPI_NSS_SOFT;
    #if (PRINTER_IS_PRUSA_XL || PRINTER_IS_PRUSA_MK4 || PRINTER_IS_PRUSA_MK3_5)
    hspi6.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
    #else
    hspi6.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
    #endif
    hspi6.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi6.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi6.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi6.Init.CRCPolynomial = 10;
    if (HAL_SPI_Init(&hspi6) != HAL_OK) {
        Error_Handler();
    }
}
#endif

void hw_tim1_init() {
    TIM_ClockConfigTypeDef sClockSourceConfig {};
    TIM_MasterConfigTypeDef sMasterConfig {};
    TIM_OC_InitTypeDef sConfigOC {};
    TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig {};

    htim1.Instance = TIM1;
    htim1.Init.Prescaler = TIM1_default_Prescaler; // 0x3fff was 100;
    htim1.Init.CounterMode = TIM_COUNTERMODE_DOWN;
    htim1.Init.Period = TIM1_default_Period;       // 0xff was 42000;
    htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim1.Init.RepetitionCounter = 0;
    if (HAL_TIM_Base_Init(&htim1) != HAL_OK) {
        Error_Handler();
    }
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_TIM_PWM_Init(&htim1) != HAL_OK) {
        Error_Handler();
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK) {
        Error_Handler();
    }
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
    sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
    if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK) { //_PWM_FAN1
        Error_Handler();
    }
    if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_2) != HAL_OK) { //_PWM_FAN
        Error_Handler();
    }
    sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
    sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
    sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
    sBreakDeadTimeConfig.DeadTime = 0;
    sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
    sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
    sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
    if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK) {
        Error_Handler();
    }
}

void hw_tim2_init() {
    TIM_ClockConfigTypeDef sClockSourceConfig {};
    TIM_MasterConfigTypeDef sMasterConfig {};
    TIM_OC_InitTypeDef sConfigOC {};

    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 100;
    htim2.Init.CounterMode = TIM_COUNTERMODE_DOWN;
    htim2.Init.Period = 42000;
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    if (HAL_TIM_Base_Init(&htim2) != HAL_OK) {
        Error_Handler();
    }
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_TIM_PWM_Init(&htim2) != HAL_OK) {
        Error_Handler();
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK) {
        Error_Handler();
    }
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 21000;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK) {
        Error_Handler();
    }

    HAL_TIM_MspPostInit(&htim2);
}

void hw_tim3_init() {
    TIM_ClockConfigTypeDef sClockSourceConfig {};
    TIM_MasterConfigTypeDef sMasterConfig {};
    TIM_OC_InitTypeDef sConfigOC {};

    htim3.Instance = TIM3;
#if (PRINTER_IS_PRUSA_MK4 || PRINTER_IS_PRUSA_MK3_5 || PRINTER_IS_PRUSA_iX)
    htim3.Init.Prescaler = 11; // 36us, 33.0kHz
#else
    htim3.Init.Prescaler = TIM3_default_Prescaler; // 49ms, 20.3Hz
#endif
    htim3.Init.CounterMode = TIM_COUNTERMODE_DOWN;
    htim3.Init.Period = TIM3_default_Period; // 0xff was 42000
    htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    if (HAL_TIM_Base_Init(&htim3) != HAL_OK) {
        Error_Handler();
    }
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_TIM_PWM_Init(&htim3) != HAL_OK) {
        Error_Handler();
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK) {
        Error_Handler();
    }
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 21000;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_3) != HAL_OK) { //_PWM_HEATER_BED
        Error_Handler();
    }
    if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_4) != HAL_OK) { //_PWM_HEATER_0
        Error_Handler();
    }

    HAL_TIM_MspPostInit(&htim3);
}

void hw_tim14_init() {
    htim14.Instance = TIM14;
    htim14.Init.Prescaler = 84;
    htim14.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim14.Init.Period = 1000;
    htim14.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    if (HAL_TIM_Base_Init(&htim14) != HAL_OK) {
        Error_Handler();
    }
    HAL_TIM_Base_Start_IT(&htim14);
}
