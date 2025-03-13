#include <buddy/main.h>
#include "printers.h"
#include <device/board.h>
#include <logging/log.hpp>
// #include "FreeRTOSConfig.h"
#include <device/peripherals.h>
#include <buddy/priorities_config.h>
#include <option/has_burst_stepping.h>

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

#if BOARD_IS_BUDDY()
static DMA_HandleTypeDef hdma_spi2_rx;
static DMA_HandleTypeDef hdma_spi2_tx;
static DMA_HandleTypeDef hdma_spi3_rx;
static DMA_HandleTypeDef hdma_spi3_tx;
static DMA_HandleTypeDef hdma_usart1_rx;
static DMA_HandleTypeDef hdma_usart2_rx;
static DMA_HandleTypeDef hdma_usart6_rx;
static DMA_HandleTypeDef hdma_usart6_tx;
static DMA_HandleTypeDef hdma_adc1;
static DMA_HandleTypeDef hdma_tim8;
#elif BOARD_IS_XBUDDY()
static DMA_HandleTypeDef hdma_spi2_rx;
static DMA_HandleTypeDef hdma_spi2_tx;
static DMA_HandleTypeDef hdma_spi3_rx;
static DMA_HandleTypeDef hdma_spi3_tx;
static DMA_HandleTypeDef hdma_spi4_tx;
static DMA_HandleTypeDef hdma_spi5_tx;
static DMA_HandleTypeDef hdma_spi5_rx;
static DMA_HandleTypeDef hdma_spi6_tx;
static DMA_HandleTypeDef hdma_usart6_rx;
static DMA_HandleTypeDef hdma_usart6_tx;
static DMA_HandleTypeDef hdma_uart8_rx;
static DMA_HandleTypeDef hdma_uart8_tx;
static DMA_HandleTypeDef hdma_adc1;
static DMA_HandleTypeDef hdma_adc3;
static DMA_HandleTypeDef hdma_tim8;
#elif BOARD_IS_XLBUDDY()
static DMA_HandleTypeDef hdma_spi3_rx;
static DMA_HandleTypeDef hdma_spi3_tx;
    #if !HAS_BURST_STEPPING()
static DMA_HandleTypeDef hdma_spi4_tx;
    #endif
static DMA_HandleTypeDef hdma_spi5_tx;
static DMA_HandleTypeDef hdma_spi5_rx;
static DMA_HandleTypeDef hdma_spi6_tx;
static DMA_HandleTypeDef hdma_usart3_rx;
static DMA_HandleTypeDef hdma_usart3_tx;
static DMA_HandleTypeDef hdma_usart6_rx;
static DMA_HandleTypeDef hdma_usart6_tx;
static DMA_HandleTypeDef hdma_uart8_rx;
static DMA_HandleTypeDef hdma_uart8_tx;
static DMA_HandleTypeDef hdma_adc1;
static DMA_HandleTypeDef hdma_adc3;
static DMA_HandleTypeDef hdma_tim8;
#else
    #error "Unknown board"
#endif

/**
 * Initializes the Global MSP.
 */
void HAL_MspInit(void) {
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();

    HAL_EnableCompensationCell(); // allows maximum slew rate to be achieved

    /* System interrupt init*/
    /* PendSV_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(PendSV_IRQn, ISR_PRIORITY_PENDSV, 0);
}

/**
 * @brief Initialize GPIO pin(s) as analog input.
 * Must be called separately for each port.
 * Does not initialize the ADC itself. Must be initialized separately.
 * @param GPIOx GPIO port of all pins
 * @param pin_mask Binary mask of pins
 */
void analog_gpio_init(GPIO_TypeDef *GPIOx, uint32_t pin_mask) {
    GPIO_InitTypeDef GPIO_InitStruct {};
    GPIO_InitStruct.Pin = pin_mask;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOx, &GPIO_InitStruct);
}

/**
 * @brief Initialize DMA for circular ADC to RAM transfer
 *
 * @param dma
 * @param dma_stream
 * @param dma_channel
 */
void adc_dma_init(DMA_HandleTypeDef *dma, DMA_Stream_TypeDef *dma_stream, uint32_t dma_channel) {
    dma->Instance = dma_stream;
    dma->Init.Channel = dma_channel;
    dma->Init.Direction = DMA_PERIPH_TO_MEMORY;
    dma->Init.PeriphInc = DMA_PINC_DISABLE;
    dma->Init.MemInc = DMA_MINC_ENABLE;
    dma->Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    dma->Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    dma->Init.Mode = DMA_CIRCULAR;
    dma->Init.Priority = DMA_PRIORITY_LOW;
    dma->Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(dma) != HAL_OK) {
        Error_Handler();
    }
}

/**
 * @brief ADC MSP Initialization
 * This function configures the hardware resources used in this example
 * @param hadc: ADC handle pointer
 * @retval None
 */
void HAL_ADC_MspInit(ADC_HandleTypeDef *hadc) {

    if (hadc->Instance == ADC1) {
        /* Peripheral clock enable */
        __HAL_RCC_ADC1_CLK_ENABLE();

        __HAL_RCC_GPIOC_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();

        /**
         * ADC1 GPIO Configuration
         * PC0     ------> ADC1_IN10
         * PA3     ------> ADC1_IN3
         * PA4     ------> ADC1_IN4
         * PA5     ------> ADC1_IN5
         * PA6     ------> ADC1_IN6
         */

#if (BOARD_IS_XBUDDY() && PRINTER_IS_PRUSA_MK3_5())
        analog_gpio_init(GPIOA, THERM_1_Pin | HEATER_VOLTAGE_Pin | BED_VOLTAGE_Pin); /*Initialize GPIOA pins as analog input*/
        analog_gpio_init(THERM_0_GPIO_Port, THERM_0_Pin);
#elif BOARD_IS_XBUDDY()
        analog_gpio_init(GPIOA, THERM_1_Pin | THERM_HEATBREAK_Pin | HEATER_VOLTAGE_Pin | BED_VOLTAGE_Pin); /*Initialize GPIOA pins as analog input*/
        analog_gpio_init(THERM_0_GPIO_Port, THERM_0_Pin); /*Initialize GPIOC pins as analog input*/
#elif BOARD_IS_BUDDY()
        analog_gpio_init(GPIOA, BED_MON_Pin | THERM_1_Pin | THERM_2_Pin | THERM_PINDA_Pin); /*Initialize GPIOA pins as analog input*/
        analog_gpio_init(THERM_0_GPIO_Port, THERM_0_Pin); /*Initialize GPIOC pins as analog input*/
#elif BOARD_IS_XLBUDDY()
        analog_gpio_init(GPIOA, GPIO_PIN_4 | GPIO_PIN_5);
        analog_gpio_init(GPIOB, GPIO_PIN_0);
#else
    #error "macro BOARD_TYPE is not defined"
#endif

        /* ADC1 DMA Init */
        adc_dma_init(&hdma_adc1, DMA2_Stream4, DMA_CHANNEL_0);

        /*Link ADC to DMA stram+channel*/
        __HAL_LINKDMA(hadc, DMA_Handle, hdma_adc1);
    }
#if (BOARD_IS_XBUDDY() || BOARD_IS_XLBUDDY())
    if (hadc->Instance == ADC3) {
        /* Peripheral clock enable */
        __HAL_RCC_ADC3_CLK_ENABLE();

        __HAL_RCC_GPIOF_CLK_ENABLE();
        /**
         * ADC3 GPIO Configuration
         * PF3     ------> ADC3_IN9
         * PF4     ------> ADC3_IN14
         * PF5     ------> ADC3_IN15
         * PF6     ------> ADC3_IN4
         * PF10     ------> ADC3_IN8
         */
        /*Initialize GPIOF pins as analog input*/
    #if BOARD_IS_XBUDDY()
        // Note: when PRINTER_IS_PRUSA_COREONE() this also initializes door sensor (THERM3_Pin == GPIO_PIN_5)
        analog_gpio_init(GPIOF, HEATER_CURRENT_Pin | INPUT_CURRENT_Pin | THERM3_Pin | MMU_CURRENT_Pin | THERM_2_Pin);
    #elif BOARD_IS_XLBUDDY()
        analog_gpio_init(GPIOF, GPIO_PIN_10);
        analog_gpio_init(GPIOC, GPIO_PIN_0);
        analog_gpio_init(GPIOF, GPIO_PIN_6);
    #endif
        /* ADC3 DMA Init */
        adc_dma_init(&hdma_adc3, DMA2_Stream0, DMA_CHANNEL_2);

        /*Link ADC to DMA stram+channel*/
        __HAL_LINKDMA(hadc, DMA_Handle, hdma_adc3);
    }
#endif
}

/**
 * @brief ADC MSP De-Initialization
 * This function freeze the hardware resources used in this example
 * @param hadc: ADC handle pointer
 * @retval None
 */
void HAL_ADC_MspDeInit(ADC_HandleTypeDef *hadc) {

    if (hadc->Instance == ADC1) {
        /* Peripheral clock disable */
        __HAL_RCC_ADC1_CLK_DISABLE();

        /**
         * ADC1 GPIO Configuration
         * PC0     ------> ADC1_IN10
         * PA3     ------> ADC1_IN3
         * PA4     ------> ADC1_IN4
         * PA5     ------> ADC1_IN5
         * PA6     ------> ADC1_IN6
         */
        HAL_GPIO_DeInit(THERM_0_GPIO_Port, THERM_0_Pin);

        HAL_GPIO_DeInit(GPIOA, BED_MON_Pin | THERM_1_Pin | THERM_2_Pin
#if (BOARD_IS_BUDDY())
                | THERM_PINDA_Pin
#endif
#if (BOARD_IS_XBUDDY() && (!PRINTER_IS_PRUSA_MK3_5()))
                | THERM_HEATBREAK_Pin
#endif
        );

        HAL_DMA_DeInit(hadc->DMA_Handle);
    }
#if (BOARD_IS_BUDDY() || BOARD_IS_XBUDDY())
    if (hadc->Instance == ADC2) {
        /* Peripheral clock disable */
        __HAL_RCC_ADC2_CLK_DISABLE();

        /**
         * ADC1 GPIO Configuration
         * PC0     ------> ADC1_IN10
         * PA3     ------> ADC1_IN3
         * PA4     ------> ADC1_IN4
         * PA5     ------> ADC1_IN5
         * PA6     ------> ADC1_IN6
         */
        HAL_GPIO_DeInit(GPIOA, HW_IDENTIFY_Pin | THERM_2_Pin);

        /* ADC1 DMA DeInit */
        HAL_DMA_DeInit(hadc->DMA_Handle);
    }
#endif
}

/**
 * @brief I2C MSP Initialization
 * This function configures the hardware resources used in this example
 * Currently replaced by hw_i2cX_pins_init functions in peripherals.cpp
 * It calls those functions to preserve functionality of HAL_I2C_Init
 * @param hi2c: I2C handle pointer
 * @retval None
 */
void HAL_I2C_MspInit(I2C_HandleTypeDef *hi2c) {
#if HAS_I2CN(1)
    if (hi2c->Instance == I2C1) {
        hw_i2c1_pins_init();
    }
#endif
#if HAS_I2CN(2)
    if (hi2c->Instance == I2C2) {
        hw_i2c2_pins_init();
    }
#endif
#if HAS_I2CN(3)
    if (hi2c->Instance == I2C3) {
        hw_i2c3_pins_init();
    }
#endif
}

/**
 * @brief I2C MSP De-Initialization
 * This function freeze the hardware resources used in this example
 * @param hi2c: I2C handle pointer
 * @retval None
 */
void HAL_I2C_MspDeInit(I2C_HandleTypeDef *hi2c) {

#if (BOARD_IS_XBUDDY() || BOARD_IS_XLBUDDY())
    if (hi2c->Instance == I2C2) {
        /* Peripheral clock disable */
        __HAL_RCC_I2C2_CLK_DISABLE();

        /**
         * I2C2 GPIO Configuration
         * PF0     ------> I2C2_SDA
         * PF1     ------> I2C2_SCL
         */
        HAL_GPIO_DeInit(GPIOF, GPIO_PIN_0);

        HAL_GPIO_DeInit(GPIOF, GPIO_PIN_1);

    }

    #if HAS_I2CN(3)
    else if (hi2c->Instance == I2C3) {
        /* Peripheral clock disable */
        __HAL_RCC_I2C3_CLK_DISABLE();

        /**
         * I2C3 GPIO Configuration
         * PC9     ------> I2C3_SDA
         * PA8     ------> I2C3_SCL
         */
        HAL_GPIO_DeInit(GPIOC, GPIO_PIN_9);

        HAL_GPIO_DeInit(GPIOA, GPIO_PIN_8);
    }
    #endif
#endif

#if (BOARD_IS_BUDDY() || BOARD_IS_XLBUDDY())
    if (hi2c->Instance == I2C1) {
        /* Peripheral clock disable */
        __HAL_RCC_I2C1_CLK_DISABLE();

        /**
         * I2C1 GPIO Configuration
         * PB8     ------> I2C1_SCL
         * PB9     ------> I2C1_SDA
         */
        HAL_GPIO_DeInit(GPIOB, GPIO_PIN_8 | GPIO_PIN_9);
    }
#endif
}

/**
 * @brief RTC MSP Initialization
 * This function configures the hardware resources used in this example
 * @param hrtc: RTC handle pointer
 * @retval None
 */
void HAL_RTC_MspInit(RTC_HandleTypeDef *hrtc) {
    if (hrtc->Instance == RTC) {
        /* Peripheral clock enable */
        __HAL_RCC_RTC_ENABLE();
    }
}

/**
 * @brief RTC MSP De-Initialization
 * This function freeze the hardware resources used in this example
 * @param hrtc: RTC handle pointer
 * @retval None
 */
void HAL_RTC_MspDeInit(RTC_HandleTypeDef *hrtc) {
    if (hrtc->Instance == RTC) {
        /* Peripheral clock disable */
        __HAL_RCC_RTC_DISABLE();
    }
}

/**
 * @brief SPI MSP Initialization
 * This function configures the hardware resources used in this example
 * @param hspi: SPI handle pointer
 * @retval None
 */
void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi) {

    GPIO_InitTypeDef GPIO_InitStruct {};
#if (BOARD_IS_XBUDDY() || BOARD_IS_XLBUDDY())
    if (hspi->Instance == SPI2) {
        /* Peripheral clock enable */
        __HAL_RCC_SPI2_CLK_ENABLE();

        __HAL_RCC_GPIOC_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();
        /**
         * SPI2 GPIO Configuration
         * PC2     ------> SPI2_MISO
         * PC3     ------> SPI2_MOSI
         * PB10     ------> SPI2_SCK
         */
        GPIO_InitStruct.Pin = GPIO_PIN_2;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;

    #if BOARD_IS_XBUDDY()
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
        GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    #else
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
    #endif
        GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
        HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

        GPIO_InitStruct.Pin = GPIO_PIN_3;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
    #if BOARD_IS_XBUDDY()
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
    #else
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    #endif
        GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
        HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

        GPIO_InitStruct.Pin = GPIO_PIN_10;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    #if spi_accelerometer == 2
        // Accelerometer CLK: Pullup. The SPI clock on the MCU is high Z when
        // not transmitting. The accelerometer requires clock idle to be high,
        // and on the accelerometer (rev. 06) there's a 100k pulldown. Set the
        // pullup on the MCU to force the clock high for the start of the
        // communication.
        GPIO_InitStruct.Pull = GPIO_PULLUP;
    #else
        GPIO_InitStruct.Pull = GPIO_NOPULL;
    #endif
    #if BOARD_IS_XBUDDY()
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
    #else
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    #endif
        GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    #if BOARD_IS_XBUDDY()
        /* SPI2 DMA Init */
        /* SPI2_RX Init */
        hdma_spi2_rx.Instance = DMA1_Stream3;
        hdma_spi2_rx.Init.Channel = DMA_CHANNEL_0;
        hdma_spi2_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
        hdma_spi2_rx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_spi2_rx.Init.MemInc = DMA_MINC_ENABLE;
        hdma_spi2_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_spi2_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdma_spi2_rx.Init.Mode = DMA_NORMAL;
        hdma_spi2_rx.Init.Priority = DMA_PRIORITY_LOW;
        hdma_spi2_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
        if (HAL_DMA_Init(&hdma_spi2_rx) != HAL_OK) {
            Error_Handler();
        }

        __HAL_LINKDMA(hspi, hdmarx, hdma_spi2_rx);

        /* SPI2 DMA Init */
        /* SPI2_TX Init */
        hdma_spi2_tx.Instance = DMA1_Stream4;
        hdma_spi2_tx.Init.Channel = DMA_CHANNEL_0;
        hdma_spi2_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
        hdma_spi2_tx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_spi2_tx.Init.MemInc = DMA_MINC_ENABLE;
        hdma_spi2_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_spi2_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdma_spi2_tx.Init.Mode = DMA_NORMAL;
        hdma_spi2_tx.Init.Priority = DMA_PRIORITY_MEDIUM;
        hdma_spi2_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
        if (HAL_DMA_Init(&hdma_spi2_tx) != HAL_OK) {
            Error_Handler();
        }

        __HAL_LINKDMA(hspi, hdmatx, hdma_spi2_tx);
    #endif

    } else if (hspi->Instance == SPI3) {
        /* Peripheral clock enable */
        __HAL_RCC_SPI3_CLK_ENABLE();

        __HAL_RCC_GPIOC_CLK_ENABLE();
        /**
         * SPI3 GPIO Configuration
         * PC10     ------> SPI3_SCK
         * PC11     ------> SPI3_MISO
         * PC12     ------> SPI3_MOSI
         */
        GPIO_InitStruct.Pin = GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_PULLDOWN;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;
        HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

        /* SPI3 DMA Init */
        /* SPI3_RX Init */
        hdma_spi3_rx.Instance = DMA1_Stream0;
        hdma_spi3_rx.Init.Channel = DMA_CHANNEL_0;
        hdma_spi3_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
        hdma_spi3_rx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_spi3_rx.Init.MemInc = DMA_MINC_ENABLE;
        hdma_spi3_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_spi3_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdma_spi3_rx.Init.Mode = DMA_NORMAL;
        hdma_spi3_rx.Init.Priority = DMA_PRIORITY_LOW;
        hdma_spi3_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
        if (HAL_DMA_Init(&hdma_spi3_rx) != HAL_OK) {
            Error_Handler();
        }

        __HAL_LINKDMA(hspi, hdmarx, hdma_spi3_rx);

        /* SPI3_TX Init */
        hdma_spi3_tx.Instance = DMA1_Stream5;
        hdma_spi3_tx.Init.Channel = DMA_CHANNEL_0;
        hdma_spi3_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
        hdma_spi3_tx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_spi3_tx.Init.MemInc = DMA_MINC_ENABLE;
        hdma_spi3_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_spi3_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdma_spi3_tx.Init.Mode = DMA_NORMAL;
        hdma_spi3_tx.Init.Priority = DMA_PRIORITY_LOW;
        hdma_spi3_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
        if (HAL_DMA_Init(&hdma_spi3_tx) != HAL_OK) {
            Error_Handler();
        }

        __HAL_LINKDMA(hspi, hdmatx, hdma_spi3_tx);

        HAL_NVIC_SetPriority(SPI3_IRQn, ISR_PRIORITY_DEFAULT, 0);
        HAL_NVIC_EnableIRQ(SPI3_IRQn);
    } else if (hspi->Instance == SPI4) {
        /* Peripheral clock enable */
        __HAL_RCC_SPI4_CLK_ENABLE();

        __HAL_RCC_GPIOE_CLK_ENABLE();
        /**
         * SPI4 GPIO Configuration
         * PE2     ------> SPI4_SCK
         * PE5     ------> SPI4_MISO
         * PE6     ------> SPI4_MOSI
         */
        GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_5 | GPIO_PIN_6;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF5_SPI4;
        HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    #if HAS_BURST_STEPPING()
        HAL_NVIC_SetPriority(SPI4_IRQn, ISR_PRIORITY_DEFAULT, 0);
        HAL_NVIC_EnableIRQ(SPI4_IRQn);
    #else
        /* SPI4_TX Init */
        hdma_spi4_tx.Instance = DMA2_Stream1;
        hdma_spi4_tx.Init.Channel = DMA_CHANNEL_4;
        hdma_spi4_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
        hdma_spi4_tx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_spi4_tx.Init.MemInc = DMA_MINC_ENABLE;
        hdma_spi4_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_spi4_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdma_spi4_tx.Init.Mode = DMA_NORMAL;
        hdma_spi4_tx.Init.Priority = DMA_PRIORITY_LOW;
        hdma_spi4_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
        if (HAL_DMA_Init(&hdma_spi4_tx) != HAL_OK) {
            Error_Handler();
        }

        __HAL_LINKDMA(hspi, hdmatx, hdma_spi4_tx);
    #endif // HAS_PHASE_STEPPING()
    } else if (hspi->Instance == SPI5) {
        /* Peripheral clock enable */
        __HAL_RCC_SPI5_CLK_ENABLE();
        __HAL_RCC_GPIOF_CLK_ENABLE();

        /**
         * SPI5 GPIO Configuration
         * PF7     ------> SPI5_SCK
         * PF8     ------> SPI5_MISO
         * PF9     ------> SPI5_MOSI
         */
        GPIO_InitStruct.Pin = GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF5_SPI5;
        HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

        /* SPI5 DMA Init */
        /* SPI5_TX Init */
        hdma_spi5_tx.Instance = DMA2_Stream6;
        hdma_spi5_tx.Init.Channel = DMA_CHANNEL_7;
        hdma_spi5_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
        hdma_spi5_tx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_spi5_tx.Init.MemInc = DMA_MINC_ENABLE;
        hdma_spi5_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_spi5_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdma_spi5_tx.Init.Mode = DMA_NORMAL;
        hdma_spi5_tx.Init.Priority = DMA_PRIORITY_MEDIUM;
        hdma_spi5_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
        if (HAL_DMA_Init(&hdma_spi5_tx) != HAL_OK) {
            Error_Handler();
        }
        __HAL_LINKDMA(hspi, hdmatx, hdma_spi5_tx);

        /* SPI5_RX Init */
        hdma_spi5_rx.Instance = DMA2_Stream3;
        hdma_spi5_rx.Init.Channel = DMA_CHANNEL_2;
        hdma_spi5_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
        hdma_spi5_rx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_spi5_rx.Init.MemInc = DMA_MINC_ENABLE;
        hdma_spi5_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_spi5_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdma_spi5_rx.Init.Mode = DMA_NORMAL;
        hdma_spi5_rx.Init.Priority = DMA_PRIORITY_LOW;
        hdma_spi5_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
        if (HAL_DMA_Init(&hdma_spi5_rx) != HAL_OK) {
            Error_Handler();
        }

        __HAL_LINKDMA(hspi, hdmarx, hdma_spi5_rx);

    } else if (hspi->Instance == SPI6) {
        /* Peripheral clock enable */
        __HAL_RCC_SPI6_CLK_ENABLE();

        __HAL_RCC_GPIOG_CLK_ENABLE();
        /**
         * SPI6 GPIO Configuration
         * PG12     ------> SPI6_MISO
         * PG13     ------> SPI6_SCK
         * PG14     ------> SPI6_MOSI
         */
        GPIO_InitStruct.Pin = GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_PULLDOWN;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF5_SPI6;
        HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

        /* SPI6 DMA Init */
        /* SPI6_TX Init */
        hdma_spi6_tx.Instance = DMA2_Stream5;
        hdma_spi6_tx.Init.Channel = DMA_CHANNEL_1;
        hdma_spi6_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
        hdma_spi6_tx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_spi6_tx.Init.MemInc = DMA_MINC_ENABLE;
        hdma_spi6_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_spi6_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdma_spi6_tx.Init.Mode = DMA_NORMAL;
        hdma_spi6_tx.Init.Priority = DMA_PRIORITY_LOW;
        hdma_spi6_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
        if (HAL_DMA_Init(&hdma_spi6_tx) != HAL_OK) {
            Error_Handler();
        }

        __HAL_LINKDMA(hspi, hdmatx, hdma_spi6_tx);
    }

#elif (BOARD_IS_BUDDY())

    if (hspi->Instance == SPI2) {
        /* Peripheral clock enable */
        __HAL_RCC_SPI2_CLK_ENABLE();

        __HAL_RCC_GPIOC_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();

        /**
         * SPI2 GPIO Configuration
         * PC2     ------> SPI2_MISO
         * PC3     ------> SPI2_MOSI
         * PB10     ------> SPI2_SCK
         */
        GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
        HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

        GPIO_InitStruct.Pin = GPIO_PIN_10;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

        /* SPI2 DMA Init */
        /* SPI2_TX Init */
        hdma_spi2_tx.Instance = DMA1_Stream4;
        hdma_spi2_tx.Init.Channel = DMA_CHANNEL_0;
        hdma_spi2_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
        hdma_spi2_tx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_spi2_tx.Init.MemInc = DMA_MINC_ENABLE;
        hdma_spi2_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_spi2_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdma_spi2_tx.Init.Mode = DMA_NORMAL;
        hdma_spi2_tx.Init.Priority = DMA_PRIORITY_MEDIUM;
        hdma_spi2_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
        if (HAL_DMA_Init(&hdma_spi2_tx) != HAL_OK) {
            Error_Handler();
        }

        __HAL_LINKDMA(hspi, hdmatx, hdma_spi2_tx);

        /* SPI2_RX Init */
        hdma_spi2_rx.Instance = DMA1_Stream3;
        hdma_spi2_rx.Init.Channel = DMA_CHANNEL_0;
        hdma_spi2_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
        hdma_spi2_rx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_spi2_rx.Init.MemInc = DMA_MINC_ENABLE;
        hdma_spi2_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_spi2_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdma_spi2_rx.Init.Mode = DMA_NORMAL;
        hdma_spi2_rx.Init.Priority = DMA_PRIORITY_LOW;
        hdma_spi2_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
        if (HAL_DMA_Init(&hdma_spi2_rx) != HAL_OK) {
            Error_Handler();
        }

        __HAL_LINKDMA(hspi, hdmarx, hdma_spi2_rx);

    } else if (hspi->Instance == SPI3) {
        /* Peripheral clock enable */
        __HAL_RCC_SPI3_CLK_ENABLE();

        __HAL_RCC_GPIOC_CLK_ENABLE();

        /**
         * SPI3 GPIO Configuration
         * PC10     ------> SPI3_SCK
         * PC11     ------> SPI3_MISO
         * PC12     ------> SPI3_MOSI
         */
        GPIO_InitStruct.Pin = FLASH_SCK_Pin | FLASH_MISO_Pin | FLASH_MOSI_Pin;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;
        HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

        /* SPI3 DMA Init */
        /* SPI3_TX Init */
        hdma_spi3_tx.Instance = DMA1_Stream7;
        hdma_spi3_tx.Init.Channel = DMA_CHANNEL_0;
        hdma_spi3_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
        hdma_spi3_tx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_spi3_tx.Init.MemInc = DMA_MINC_ENABLE;
        hdma_spi3_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_spi3_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdma_spi3_tx.Init.Mode = DMA_NORMAL;
        hdma_spi3_tx.Init.Priority = DMA_PRIORITY_MEDIUM;
        hdma_spi3_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
        if (HAL_DMA_Init(&hdma_spi3_tx) != HAL_OK) {
            Error_Handler();
        }
        __HAL_LINKDMA(hspi, hdmatx, hdma_spi3_tx);

        /* SPI3_RX Init */
        hdma_spi3_rx.Instance = DMA1_Stream0;
        hdma_spi3_rx.Init.Channel = DMA_CHANNEL_0;
        hdma_spi3_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
        hdma_spi3_rx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_spi3_rx.Init.MemInc = DMA_MINC_ENABLE;
        hdma_spi3_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_spi3_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdma_spi3_rx.Init.Mode = DMA_NORMAL;
        hdma_spi3_rx.Init.Priority = DMA_PRIORITY_LOW;
        hdma_spi3_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
        if (HAL_DMA_Init(&hdma_spi3_rx) != HAL_OK) {
            Error_Handler();
        }

        __HAL_LINKDMA(hspi, hdmarx, hdma_spi3_rx);
    }

#else
    #error "Unknown board."
#endif
}

/**
 * @brief SPI MSP De-Initialization
 * This function freeze the hardware resources used in this example
 * @param hspi: SPI handle pointer
 * @retval None
 */
void HAL_SPI_MspDeInit(SPI_HandleTypeDef *hspi) {
#if (BOARD_IS_XBUDDY() || BOARD_IS_XLBUDDY())
    if (hspi->Instance == SPI2) {
        /* Peripheral clock disable */
        __HAL_RCC_SPI2_CLK_DISABLE();

        /**
         * SPI2 GPIO Configuration
         * PC2     ------> SPI2_MISO
         * PC3     ------> SPI2_MOSI
         * PB10     ------> SPI2_SCK
         */
        HAL_GPIO_DeInit(GPIOC, GPIO_PIN_2 | GPIO_PIN_3);

        HAL_GPIO_DeInit(GPIOB, GPIO_PIN_10);
    } else if (hspi->Instance == SPI3) {
        /* Peripheral clock disable */
        __HAL_RCC_SPI3_CLK_DISABLE();

        /**
         * SPI3 GPIO Configuration
         * PC10     ------> SPI3_SCK
         * PC11     ------> SPI3_MISO
         * PC12     ------> SPI3_MOSI
         */
        HAL_GPIO_DeInit(GPIOC, GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12);

        /* SPI3 DMA DeInit */
        HAL_DMA_DeInit(hspi->hdmarx);
        HAL_DMA_DeInit(hspi->hdmatx);

        HAL_NVIC_DisableIRQ(SPI3_IRQn);
    } else if (hspi->Instance == SPI4) {
        /* Peripheral clock disable */
        __HAL_RCC_SPI4_CLK_DISABLE();

        /**
         * SPI4 GPIO Configuration
         * PE2     ------> SPI4_SCK
         * PE5     ------> SPI4_MISO
         * PE6     ------> SPI4_MOSI
         */
        HAL_GPIO_DeInit(GPIOE, GPIO_PIN_2 | GPIO_PIN_5 | GPIO_PIN_6);
    } else if (hspi->Instance == SPI5) {
        /* Peripheral clock disable */
        __HAL_RCC_SPI5_CLK_DISABLE();

        /**
         * SPI5 GPIO Configuration
         * PF7     ------> SPI5_SCK
         * PF8     ------> SPI5_MISO
         * PF9     ------> SPI5_MOSI
         */
        HAL_GPIO_DeInit(GPIOF, GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9);
    } else if (hspi->Instance == SPI6) {
        /* Peripheral clock disable */
        __HAL_RCC_SPI6_CLK_DISABLE();

        /**SPI6 GPIO Configuration
    PG12     ------> SPI6_MISO
    PG13     ------> SPI6_SCK
    PG14     ------> SPI6_MOSI
    */
        HAL_GPIO_DeInit(GPIOG, GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14);
        HAL_DMA_DeInit(hspi->hdmatx);
        HAL_DMA_DeInit(hspi->hdmarx);
    }

#elif (BOARD_IS_BUDDY())

    if (hspi->Instance == SPI2) {
        /* Peripheral clock disable */
        __HAL_RCC_SPI2_CLK_DISABLE();

        /**
         * SPI2 GPIO Configuration
         * PC2     ------> SPI2_MISO
         * PC3     ------> SPI2_MOSI
         * PB10     ------> SPI2_SCK
         */
        HAL_GPIO_DeInit(GPIOC, GPIO_PIN_2 | GPIO_PIN_3);

        HAL_GPIO_DeInit(GPIOB, GPIO_PIN_10);

        /* SPI2 DMA DeInit */
        HAL_DMA_DeInit(hspi->hdmatx);
        HAL_DMA_DeInit(hspi->hdmarx);
    } else if (hspi->Instance == SPI3) {
        /* Peripheral clock disable */
        __HAL_RCC_SPI3_CLK_DISABLE();

        /**
         * SPI3 GPIO Configuration
         * PC10     ------> SPI3_SCK
         * PC11     ------> SPI3_MISO
         * PC12     ------> SPI3_MOSI
         */
        HAL_GPIO_DeInit(GPIOC, FLASH_SCK_Pin | FLASH_MISO_Pin | FLASH_MOSI_Pin);
    }

#else
    #error "Unknown board."
#endif
}

/**
 * @brief TIM_Base MSP Initialization
 * This function configures the hardware resources used in this example
 * @param htim_base: TIM_Base handle pointer
 * @retval None
 */
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim_base) {

    if (htim_base->Instance == TIM1) {
        /* Peripheral clock enable */
        __HAL_RCC_TIM1_CLK_ENABLE();
    } else if (htim_base->Instance == TIM2) {
        /* Peripheral clock enable */
        __HAL_RCC_TIM2_CLK_ENABLE();
    } else if (htim_base->Instance == TIM3) {
        /* Peripheral clock enable */
        __HAL_RCC_TIM3_CLK_ENABLE();
    } else if (htim_base->Instance == TIM8) {
        /* Peripheral clock enable */
        __HAL_RCC_TIM8_CLK_ENABLE();
    } else if (htim_base->Instance == TIM9) {
        /* Peripheral clock enable */
        __HAL_RCC_TIM9_CLK_ENABLE();

        HAL_NVIC_SetPriority(TIM1_BRK_TIM9_IRQn, ISR_PRIORITY_ACCELEROMETER, 0);
        HAL_NVIC_EnableIRQ(TIM1_BRK_TIM9_IRQn);
    } else if (htim_base->Instance == TIM13) {
        /* Peripheral clock enable */
        __HAL_RCC_TIM13_CLK_ENABLE();

        HAL_NVIC_SetPriority(TIM8_UP_TIM13_IRQn, ISR_PRIORITY_PHASE_TIMER, 1);
        HAL_NVIC_EnableIRQ(TIM8_UP_TIM13_IRQn);
    } else if (htim_base->Instance == TIM14) {
        /* Peripheral clock enable */
        __HAL_RCC_TIM14_CLK_ENABLE();
        /* TIM14 interrupt Init */
        HAL_NVIC_SetPriority(TIM8_TRG_COM_TIM14_IRQn, ISR_PRIORITY_DEFAULT, 0);
        HAL_NVIC_EnableIRQ(TIM8_TRG_COM_TIM14_IRQn);
    }
}

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim) {

    GPIO_InitTypeDef GPIO_InitStruct {};
    if (htim->Instance == TIM2) {
        __HAL_RCC_GPIOA_CLK_ENABLE();

        /**
         * TIM2 GPIO Configuration
         * PA0-WKUP     ------> TIM2_CH1
         */
        GPIO_InitStruct.Pin = BUZZER_Pin;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
        HAL_GPIO_Init(BUZZER_GPIO_Port, &GPIO_InitStruct);
    } else if (htim->Instance == TIM3) {
        __HAL_RCC_GPIOB_CLK_ENABLE();

        /**
         * TIM3 GPIO Configuration
         * PB0     ------> TIM3_CH3
         * PB1     ------> TIM3_CH4
         */
        GPIO_InitStruct.Pin = BED_HEAT_Pin | HEAT0_Pin;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF2_TIM3;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    } else if (htim->Instance == TIM8) {
        /* TIM8 DMA Init */
        hdma_tim8.Instance = DMA2_Stream1;
        hdma_tim8.Init.Channel = DMA_CHANNEL_7;
        hdma_tim8.Init.Direction = DMA_MEMORY_TO_PERIPH;
        hdma_tim8.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_tim8.Init.MemInc = DMA_MINC_ENABLE;
        hdma_tim8.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
        hdma_tim8.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
        hdma_tim8.Init.Mode = DMA_NORMAL;
        hdma_tim8.Init.Priority = DMA_PRIORITY_HIGH;
        hdma_tim8.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
        if (HAL_DMA_Init(&hdma_tim8) != HAL_OK) {
            Error_Handler();
        }

        __HAL_LINKDMA(htim, hdma[TIM_DMA_ID_UPDATE], hdma_tim8);
    }
}

/**
 * @brief TIM_Base MSP De-Initialization
 * This function freeze the hardware resources used in this example
 * @param htim_base: TIM_Base handle pointer
 * @retval None
 */
void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef *htim_base) {
    if (htim_base->Instance == TIM1) {
        /* Peripheral clock disable */
        __HAL_RCC_TIM1_CLK_DISABLE();
    } else if (htim_base->Instance == TIM2) {
        /* Peripheral clock disable */
        __HAL_RCC_TIM2_CLK_DISABLE();
    } else if (htim_base->Instance == TIM3) {
        /* Peripheral clock disable */
        __HAL_RCC_TIM3_CLK_DISABLE();
    } else if (htim_base->Instance == TIM9) {
        /* Peripheral clock disable */
        __HAL_RCC_TIM9_CLK_DISABLE();
        HAL_NVIC_DisableIRQ(TIM1_BRK_TIM9_IRQn);
    } else if (htim_base->Instance == TIM13) {
        __HAL_RCC_TIM13_CLK_DISABLE();
        HAL_NVIC_DisableIRQ(TIM8_UP_TIM13_IRQn);
    } else if (htim_base->Instance == TIM14) {
        /* Peripheral clock disable */
        __HAL_RCC_TIM14_CLK_DISABLE();

        /* TIM14 interrupt DeInit */
        HAL_NVIC_DisableIRQ(TIM8_TRG_COM_TIM14_IRQn);
    }
}

/**
 * @brief UART MSP Initialization
 * This function configures the hardware resources used in this example
 * @param huart: UART handle pointer
 * @retval None
 */
void HAL_UART_MspInit(UART_HandleTypeDef *huart) {
    GPIO_InitTypeDef GPIO_InitStruct {};

#if BOARD_IS_BUDDY()
    if (huart->Instance == USART1) {
        /* Peripheral clock enable */
        __HAL_RCC_USART1_CLK_ENABLE();

        __HAL_RCC_GPIOB_CLK_ENABLE();
        /**
         * USART1 GPIO Configuration
         * PB6     ------> USART1_TX
         * PB7     ------> USART1_RX
         */
        GPIO_InitStruct.Pin = TX1_Pin | RX1_Pin;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

        /* USART1 DMA Init */
        /* USART1_RX Init */
        hdma_usart1_rx.Instance = DMA2_Stream2;
        hdma_usart1_rx.Init.Channel = DMA_CHANNEL_4;
        hdma_usart1_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
        hdma_usart1_rx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_usart1_rx.Init.MemInc = DMA_MINC_ENABLE;
        hdma_usart1_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_usart1_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdma_usart1_rx.Init.Mode = DMA_CIRCULAR;
        hdma_usart1_rx.Init.Priority = DMA_PRIORITY_LOW;
        hdma_usart1_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
        if (HAL_DMA_Init(&hdma_usart1_rx) != HAL_OK) {
            Error_Handler();
        }

        __HAL_LINKDMA(huart, hdmarx, hdma_usart1_rx);
    } else if (huart->Instance == USART2) {
        /* Peripheral clock enable */
        __HAL_RCC_USART2_CLK_ENABLE();
        __HAL_RCC_GPIOD_CLK_ENABLE();

        /**
         * USART2 GPIO Configuration
         * PD5     ------> USART2_TX
         * PD6     ------> USART2_RX (no longer used; halfduplex)
         */
        GPIO_InitStruct.Pin = GPIO_PIN_5;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
        HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

        /* USART2 DMA Init */
        /* USART2_RX Init */
        hdma_usart2_rx.Instance = DMA1_Stream5;
        hdma_usart2_rx.Init.Channel = DMA_CHANNEL_4;
        hdma_usart2_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
        hdma_usart2_rx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_usart2_rx.Init.MemInc = DMA_MINC_ENABLE;
        hdma_usart2_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_usart2_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdma_usart2_rx.Init.Mode = DMA_CIRCULAR;
        hdma_usart2_rx.Init.Priority = DMA_PRIORITY_LOW;
        hdma_usart2_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
        if (HAL_DMA_Init(&hdma_usart2_rx) != HAL_OK) {
            Error_Handler();
        }

        // Link with DMA
        __HAL_LINKDMA(huart, hdmarx, hdma_usart2_rx);

        // Enable interrupts on the peripheral
        __HAL_UART_ENABLE_IT(huart, UART_IT_IDLE);
        __HAL_UART_ENABLE_IT(huart, UART_IT_TC);

        // Clear Transmit Complete ISR flag
        __HAL_UART_CLEAR_FLAG(huart, UART_FLAG_TC);

        // Enable the ISR
        HAL_NVIC_SetPriority(USART2_IRQn, ISR_PRIORITY_DEFAULT, 0);
        HAL_NVIC_EnableIRQ(USART2_IRQn);
    } else if (huart->Instance == USART6) { /* Peripheral clock enable */
        __HAL_RCC_USART6_CLK_ENABLE();

        __HAL_RCC_GPIOC_CLK_ENABLE();
        /**
         * USART6 GPIO Configuration
         * PC6     ------> USART6_TX
         * PC7     ------> USART6_RX
         */
        GPIO_InitStruct.Pin = ESP_TX_Pin | ESP_RX_Pin;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF8_USART6;
        HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

        /* USART6 DMA Init */
        /* USART6_RX Init */
        hdma_usart6_rx.Instance = DMA2_Stream1;
        hdma_usart6_rx.Init.Channel = DMA_CHANNEL_5;
        hdma_usart6_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
        hdma_usart6_rx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_usart6_rx.Init.MemInc = DMA_MINC_ENABLE;
        hdma_usart6_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_usart6_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdma_usart6_rx.Init.Mode = DMA_CIRCULAR;
        hdma_usart6_rx.Init.Priority = DMA_PRIORITY_LOW;
        hdma_usart6_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
        if (HAL_DMA_Init(&hdma_usart6_rx) != HAL_OK) {
            Error_Handler();
        }

        // Link with DMA
        __HAL_LINKDMA(huart, hdmarx, hdma_usart6_rx);

        /* USART6_TX Init */
        hdma_usart6_tx.Instance = DMA2_Stream6;
        hdma_usart6_tx.Init.Channel = DMA_CHANNEL_5;
        hdma_usart6_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
        hdma_usart6_tx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_usart6_tx.Init.MemInc = DMA_MINC_ENABLE;
        hdma_usart6_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_usart6_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdma_usart6_tx.Init.Mode = DMA_NORMAL;
        hdma_usart6_tx.Init.Priority = DMA_PRIORITY_LOW;
        hdma_usart6_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
        if (HAL_DMA_Init(&hdma_usart6_tx) != HAL_OK) {
            Error_Handler();
        }

        // Link with DMA
        __HAL_LINKDMA(huart, hdmatx, hdma_usart6_tx);

        // Enable interrupts on the peripheral
        __HAL_UART_ENABLE_IT(huart, UART_IT_IDLE);
        __HAL_UART_ENABLE_IT(huart, UART_IT_TC);

        // Clear Transmit Complete ISR flag
        __HAL_UART_CLEAR_FLAG(huart, UART_FLAG_TC);

        // Enable the ISR
        HAL_NVIC_SetPriority(USART6_IRQn, ISR_PRIORITY_DEFAULT, 0);
        HAL_NVIC_EnableIRQ(USART6_IRQn);
    }
#endif

#if BOARD_IS_XLBUDDY()
    if (huart->Instance == USART3) {

        __HAL_RCC_USART3_CLK_ENABLE();

        __HAL_RCC_GPIOD_CLK_ENABLE();

        /**
         * USART3 GPIO Configuration
         * PD8     ------> USART3_TX
         * PD9     ------> USART3_RX
         */
        GPIO_InitStruct.Pin = GPIO_PIN_8;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
        HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

        GPIO_InitStruct.Pin = GPIO_PIN_9;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
        HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

        /* USART3 DMA Init */
        /* USART3_RX Init */
        hdma_usart3_rx.Instance = DMA1_Stream1;
        hdma_usart3_rx.Init.Channel = DMA_CHANNEL_4;
        hdma_usart3_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
        hdma_usart3_rx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_usart3_rx.Init.MemInc = DMA_MINC_ENABLE;
        hdma_usart3_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_usart3_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdma_usart3_rx.Init.Mode = DMA_CIRCULAR;
        hdma_usart3_rx.Init.Priority = DMA_PRIORITY_LOW;
        hdma_usart3_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
        if (HAL_DMA_Init(&hdma_usart3_rx) != HAL_OK) {
            Error_Handler();
        }

        __HAL_LINKDMA(huart, hdmarx, hdma_usart3_rx);

        /* USART3_TX Init */
        hdma_usart3_tx.Instance = DMA1_Stream3;
        hdma_usart3_tx.Init.Channel = DMA_CHANNEL_4;
        hdma_usart3_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
        hdma_usart3_tx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_usart3_tx.Init.MemInc = DMA_MINC_ENABLE;
        hdma_usart3_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_usart3_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdma_usart3_tx.Init.Mode = DMA_NORMAL;
        hdma_usart3_tx.Init.Priority = DMA_PRIORITY_LOW;
        hdma_usart3_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
        if (HAL_DMA_Init(&hdma_usart3_tx) != HAL_OK) {
            Error_Handler();
        }

        __HAL_LINKDMA(huart, hdmatx, hdma_usart3_tx);

        // Enable interrupts on the peripheral
        __HAL_UART_ENABLE_IT(huart, UART_IT_IDLE);

        // Clear Transmit Complete ISR flag
        __HAL_UART_CLEAR_FLAG(huart, UART_FLAG_TC);

        HAL_NVIC_SetPriority(USART3_IRQn, ISR_PRIORITY_PUPPIES_USART, 0);
        HAL_NVIC_EnableIRQ(USART3_IRQn);
    }
#endif

#if (BOARD_IS_XBUDDY() || BOARD_IS_XLBUDDY())
    if (huart->Instance == UART8) {
        /* Peripheral clock enable */
        __HAL_RCC_UART8_CLK_ENABLE();

        __HAL_RCC_GPIOE_CLK_ENABLE();
        /**
         * UART8 GPIO Configuration
         * PE0     ------> UART8_RX
         * PE1     ------> UART8_TX
         */
        GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF8_UART8;
        HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

        /* UART8 DMA Init */
        /* UART8_RX Init */
        hdma_uart8_rx.Instance = DMA1_Stream6;
        hdma_uart8_rx.Init.Channel = DMA_CHANNEL_5;
        hdma_uart8_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
        hdma_uart8_rx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_uart8_rx.Init.MemInc = DMA_MINC_ENABLE;
        hdma_uart8_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_uart8_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdma_uart8_rx.Init.Mode = DMA_CIRCULAR;
        hdma_uart8_rx.Init.Priority = DMA_PRIORITY_LOW;
        hdma_uart8_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
        if (HAL_DMA_Init(&hdma_uart8_rx) != HAL_OK) {
            Error_Handler();
        }

        __HAL_LINKDMA(huart, hdmarx, hdma_uart8_rx);

        /* UART8_TX Init */
        hdma_uart8_tx.Instance = DMA1_Stream0;
        hdma_uart8_tx.Init.Channel = DMA_CHANNEL_5;
        hdma_uart8_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
        hdma_uart8_tx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_uart8_tx.Init.MemInc = DMA_MINC_ENABLE;
        hdma_uart8_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_uart8_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdma_uart8_tx.Init.Mode = DMA_NORMAL;
        hdma_uart8_tx.Init.Priority = DMA_PRIORITY_LOW;
        hdma_uart8_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
        if (HAL_DMA_Init(&hdma_uart8_tx) != HAL_OK) {
            Error_Handler();
        }

        __HAL_LINKDMA(huart, hdmatx, hdma_uart8_tx);

        // Enable interrupts on the peripheral
        __HAL_UART_ENABLE_IT(huart, UART_IT_IDLE);
        __HAL_UART_ENABLE_IT(huart, UART_IT_TC);

        // Clear Transmit Complete ISR flag
        __HAL_UART_CLEAR_FLAG(huart, UART_FLAG_TC);

        /* UART8 interrupt Init */
        HAL_NVIC_SetPriority(UART8_IRQn, ISR_PRIORITY_DEFAULT, 0);
        HAL_NVIC_EnableIRQ(UART8_IRQn);
    } else if (huart->Instance == USART6) {
        // log_debug(Buddy, "HAL_UART6_MspInit");

        /* Peripheral clock enable */
        __HAL_RCC_USART6_CLK_ENABLE();

        __HAL_RCC_GPIOC_CLK_ENABLE();
        /**
         * USART6 GPIO Configuration
         * PC6     ------> USART6_TX
         * PC7     ------> USART6_RX
         */
        GPIO_InitStruct.Pin = MMU_TX_Pin | MMU_RX_Pin;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF8_USART6;
        HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

        /* USART6 DMA Init */
        /* USART6_RX Init */
        hdma_usart6_rx.Instance = DMA2_Stream2;
        hdma_usart6_rx.Init.Channel = DMA_CHANNEL_5;
        hdma_usart6_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
        hdma_usart6_rx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_usart6_rx.Init.MemInc = DMA_MINC_ENABLE;
        hdma_usart6_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_usart6_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdma_usart6_rx.Init.Mode = DMA_CIRCULAR;
        hdma_usart6_rx.Init.Priority = DMA_PRIORITY_LOW;
        hdma_usart6_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
        if (HAL_DMA_Init(&hdma_usart6_rx) != HAL_OK) {
            Error_Handler();
        }

        __HAL_LINKDMA(huart, hdmarx, hdma_usart6_rx);

        /* USART6_TX Init */
        hdma_usart6_tx.Instance = DMA2_Stream7;
        hdma_usart6_tx.Init.Channel = DMA_CHANNEL_5;
        hdma_usart6_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
        hdma_usart6_tx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_usart6_tx.Init.MemInc = DMA_MINC_ENABLE;
        hdma_usart6_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
        hdma_usart6_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
        hdma_usart6_tx.Init.Mode = DMA_NORMAL;
        hdma_usart6_tx.Init.Priority = DMA_PRIORITY_LOW;
        hdma_usart6_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
        if (HAL_DMA_Init(&hdma_usart6_tx) != HAL_OK) {
            Error_Handler();
        }

        __HAL_LINKDMA(huart, hdmatx, hdma_usart6_tx);

        // Enable interrupts on the peripheral
        __HAL_UART_ENABLE_IT(huart, UART_IT_IDLE);

        // Clear Transmit Complete ISR flag
        __HAL_UART_CLEAR_FLAG(huart, UART_FLAG_TC);

        // Enable the ISR
    #if (PRINTER_IS_PRUSA_iX() || PRINTER_IS_PRUSA_COREONE())
        HAL_NVIC_SetPriority(USART6_IRQn, ISR_PRIORITY_PUPPIES_USART, 0);
    #else
        HAL_NVIC_SetPriority(USART6_IRQn, ISR_PRIORITY_DEFAULT, 0);
    #endif
        HAL_NVIC_EnableIRQ(USART6_IRQn);
    }
#endif
}

/**
 * @brief UART MSP De-Initialization
 * This function freeze the hardware resources used in this example
 * @param huart: UART handle pointer
 * @retval None
 */
void HAL_UART_MspDeInit(UART_HandleTypeDef *huart) {

#if (BOARD_IS_BUDDY())
    if (huart->Instance == USART1) {
        /* Peripheral clock disable */
        __HAL_RCC_USART1_CLK_DISABLE();

        /**
         * USART1 GPIO Configuration
         * PB6     ------> USART1_TX
         * PB7     ------> USART1_RX
         */
        HAL_GPIO_DeInit(GPIOB, TX1_Pin | RX1_Pin);

        /* USART1 DMA DeInit */
        HAL_DMA_DeInit(huart->hdmarx);

    } else if (huart->Instance == USART2) {
        /* Peripheral clock disable */
        __HAL_RCC_USART2_CLK_DISABLE();

        /**
         * USART2 GPIO Configuration
         * PD5     ------> USART2_TX
         * PD6     ------> USART2_RX
         */
        HAL_GPIO_DeInit(GPIOD, GPIO_PIN_5 | GPIO_PIN_6);

        /* USART2 DMA DeInit */
        HAL_DMA_DeInit(huart->hdmarx);
    } else if (huart->Instance == USART6) {
        /* Peripheral clock disable */
        __HAL_RCC_USART6_CLK_DISABLE();

        /**
         * USART6 GPIO Configuration
         * PC6     ------> USART6_TX
         * PC7     ------> USART6_RX
         */
        HAL_GPIO_DeInit(GPIOC, ESP_TX_Pin | ESP_RX_Pin);

        /* USART6 DMA DeInit */
        HAL_DMA_DeInit(huart->hdmarx);

        /* USART6 interrupt DeInit */
        HAL_NVIC_DisableIRQ(USART6_IRQn);
    }
#endif

#if (BOARD_IS_XBUDDY() || BOARD_IS_XLBUDDY())
    if (huart->Instance == UART8) {
        /* Peripheral clock disable */
        __HAL_RCC_UART8_CLK_DISABLE();

        /**
         * UART8 GPIO Configuration
         * PE0     ------> UART8_RX
         * PE1     ------> UART8_TX
         */
        HAL_GPIO_DeInit(GPIOE, GPIO_PIN_0 | GPIO_PIN_1);

        /* UART8 DMA DeInit */
        HAL_DMA_DeInit(huart->hdmarx);
        HAL_DMA_DeInit(huart->hdmatx);
    } else if (huart->Instance == USART6) {
        /* Peripheral clock disable */
        __HAL_RCC_USART6_CLK_DISABLE();

        /**
         * USART6 GPIO Configuration
         * PC6     ------> USART6_TX
         * PC7     ------> USART6_RX
         */
        HAL_GPIO_DeInit(GPIOC, MMU_TX_Pin | MMU_RX_Pin);

        /* USART6 DMA DeInit */
        HAL_DMA_DeInit(huart->hdmarx);

        /* USART6 interrupt DeInit */
        HAL_NVIC_DisableIRQ(USART6_IRQn);
    }
#endif

#if (BOARD_IS_XLBUDDY())
    if (huart->Instance == USART3) {
        /* Peripheral clock disable */
        __HAL_RCC_USART3_CLK_DISABLE();

        /**
         * USART3 GPIO Configuration
         * PD8     ------> USART3_TX
         * PD9     ------> USART3_RX
         */
        HAL_GPIO_DeInit(GPIOD, GPIO_PIN_8 | GPIO_PIN_9);

        /* USART3 DMA DeInit */
        HAL_DMA_DeInit(huart->hdmarx);
        HAL_DMA_DeInit(huart->hdmatx);

        HAL_NVIC_DisableIRQ(USART3_IRQn);
    }
#endif
}

/**
 * @brief RNG MSP Initialization
 * This function configures the hardware resources used in this example
 * @param hrng: RNG handle pointer
 * @retval None
 */
void HAL_RNG_MspInit(RNG_HandleTypeDef *hrng) {
    if (hrng->Instance == RNG) {
        /* Peripheral clock enable */
        __HAL_RCC_RNG_CLK_ENABLE();
    }
}

/**
 * @brief RNG MSP De-Initialization
 * This function freeze the hardware resources used in this example
 * @param hrng: RNG handle pointer
 * @retval None
 */
void HAL_RNG_MspDeInit(RNG_HandleTypeDef *hrng) {
    if (hrng->Instance == RNG) {
        /* Peripheral clock disable */
        __HAL_RCC_RNG_CLK_DISABLE();
    }
}
