#include "main.h"

extern DMA_HandleTypeDef hdma_spi2_tx;

extern DMA_HandleTypeDef hdma_spi2_rx;

extern DMA_HandleTypeDef hdma_spi3_tx;

extern DMA_HandleTypeDef hdma_spi3_rx;

extern DMA_HandleTypeDef hdma_usart1_rx;

extern DMA_HandleTypeDef hdma_usart2_rx;

extern DMA_HandleTypeDef hdma_usart6_rx;

extern DMA_HandleTypeDef hdma_adc1;

extern RNG_HandleTypeDef hrng;

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/**
 * Initializes the Global MSP.
 */
void HAL_MspInit(void) {
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();

    /* System interrupt init*/
    /* PendSV_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(PendSV_IRQn, configLIBRARY_LOWEST_INTERRUPT_PRIORITY, 0);
}

/**
 * @brief Initialize GPIO pin(s) as analog input.
 * Must be called separately for each port.
 * Does not initialize the ADC itself. Must be initialized separately.
 * @param GPIOx GPIO port of all pins
 * @param pin_mask Binary mask of pins
 */
void analog_gpio_init(GPIO_TypeDef *GPIOx, uint32_t pin_mask) {
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
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

        /**ADC1 GPIO Configuration
         * PC0     ------> ADC1_IN10
         * PA3     ------> ADC1_IN3
         * PA4     ------> ADC1_IN4
         * PA5     ------> ADC1_IN5
         * PA6     ------> ADC1_IN6
         */
        /*Initialize GPIOC pins as analog input*/
        analog_gpio_init(THERM_0_GPIO_Port, THERM_0_Pin);

        /*Initialize GPIOA pins as analog input*/
        analog_gpio_init(GPIOA, BED_MON_Pin | THERM_1_Pin | THERM_2_Pin | THERM_PINDA_Pin);

        /* ADC1 DMA Init */
        adc_dma_init(&hdma_adc1, DMA2_Stream0, DMA_CHANNEL_0);

        /*Link ADC to DMA stram+channel*/
        __HAL_LINKDMA(hadc, DMA_Handle, hdma_adc1);
    }
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

        HAL_GPIO_DeInit(GPIOA, BED_MON_Pin | THERM_1_Pin | THERM_2_Pin | THERM_PINDA_Pin);

        HAL_DMA_DeInit(hadc->DMA_Handle);
    }
}

/**
 * @brief I2C MSP Initialization
 * This function configures the hardware resources used in this example
 * @param hi2c: I2C handle pointer
 * @retval None
 */
void HAL_I2C_MspInit(I2C_HandleTypeDef *hi2c) {

    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    if (hi2c->Instance == I2C1) {
        __HAL_RCC_GPIOB_CLK_ENABLE();
        /**
         * I2C1 GPIO Configuration
         * PB8     ------> I2C1_SCL
         * PB9     ------> I2C1_SDA
         */
        GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

        /* Peripheral clock enable */
        __HAL_RCC_I2C1_CLK_ENABLE();
    }
}

/**
 * @brief I2C MSP De-Initialization
 * This function freeze the hardware resources used in this example
 * @param hi2c: I2C handle pointer
 * @retval None
 */
void HAL_I2C_MspDeInit(I2C_HandleTypeDef *hi2c) {

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

    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
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
}

/**
 * @brief SPI MSP De-Initialization
 * This function freeze the hardware resources used in this example
 * @param hspi: SPI handle pointer
 * @retval None
 */
void HAL_SPI_MspDeInit(SPI_HandleTypeDef *hspi) {

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
    } else if (htim_base->Instance == TIM14) {
        /* Peripheral clock enable */
        __HAL_RCC_TIM14_CLK_ENABLE();
        /* TIM14 interrupt Init */
        HAL_NVIC_SetPriority(TIM8_TRG_COM_TIM14_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY, 0);
        HAL_NVIC_EnableIRQ(TIM8_TRG_COM_TIM14_IRQn);
    }
}

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim) {

    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
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
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
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
        HAL_NVIC_SetPriority(USART2_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY, 0);
        HAL_NVIC_EnableIRQ(USART2_IRQn);

    } else if (huart->Instance == USART6) {
        /* Peripheral clock enable */
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

        // Enable interrupts on the peripheral
        __HAL_UART_ENABLE_IT(huart, UART_IT_IDLE);
        __HAL_UART_ENABLE_IT(huart, UART_IT_TC);

        // Clear Transmit Complete ISR flag
        __HAL_UART_CLEAR_FLAG(huart, UART_FLAG_TC);

        // Enable the ISR
        HAL_NVIC_SetPriority(USART6_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY, 0);
        HAL_NVIC_EnableIRQ(USART6_IRQn);
    }
}

/**
 * @brief UART MSP De-Initialization
 * This function freeze the hardware resources used in this example
 * @param huart: UART handle pointer
 * @retval None
 */
void HAL_UART_MspDeInit(UART_HandleTypeDef *huart) {
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
