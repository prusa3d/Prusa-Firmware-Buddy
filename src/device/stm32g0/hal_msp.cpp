#include <device/peripherals.h>
#include "bsod.h"

static void Error_Handler() {
    bsod("hal_msp");
}

void HAL_MspInit(void) {
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();

    // Disable the internal Pull-Up in Dead Battery pins of UCPD peripheral
    HAL_SYSCFG_StrobeDBattpinsConfig(SYSCFG_CFGR1_UCPD1_STROBE | SYSCFG_CFGR1_UCPD2_STROBE);
}

void HAL_ADC_MspInit(ADC_HandleTypeDef *adcHandle) {
    GPIO_InitTypeDef GPIO_InitStruct {};
    RCC_PeriphCLKInitTypeDef PeriphClkInit {};
    if (adcHandle->Instance == ADC1) {

        // Initializes the peripherals clocks
        PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
        PeriphClkInit.AdcClockSelection = RCC_ADCCLKSOURCE_SYSCLK;
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
            Error_Handler();
        }

        // ADC1 clock enable
        __HAL_RCC_ADC_CLK_ENABLE();

        __HAL_RCC_GPIOA_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();

        /**ADC1 GPIO Configuration
            PA1     ------> ADC1_IN1.....Picked 1
            PA2     ------> ADC1_IN2.....Picked 0
            PA4     ------> ADC1_IN4.....Tool Filament sensor
            PA5     ------> ADC1_IN5.....NTC 1
            PA7     ------> ADC1_IN7.....NTC internal
            PB0     ------> ADC1_IN8.....Heater current
            PB2     ------> ADC1_IN10....NTC 2
            PB10     ------> ADC1_IN11...Meas 24 V
            */

        GPIO_InitStruct.Pin = PICKED1_Pin | PICKED0_Pin | TFS_Pin | NTC_Pin | NTC_INTERNAL_Pin;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        GPIO_InitStruct.Pin = HEATER_CURRENT_Pin | NTC2_Pin | MEAS_24V_Pin;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

        // ADC1 DMA Init
        hdma_adc1.Instance = DMA1_Channel1;
        hdma_adc1.Init.Request = DMA_REQUEST_ADC1;
        hdma_adc1.Init.Direction = DMA_PERIPH_TO_MEMORY;
        hdma_adc1.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_adc1.Init.MemInc = DMA_MINC_ENABLE;
        hdma_adc1.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
        hdma_adc1.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
        hdma_adc1.Init.Mode = DMA_CIRCULAR;
        hdma_adc1.Init.Priority = DMA_PRIORITY_HIGH;
        if (HAL_DMA_Init(&hdma_adc1) != HAL_OK) {
            Error_Handler();
        }

        __HAL_LINKDMA(adcHandle, DMA_Handle, hdma_adc1);
    }
}

void HAL_ADC_MspDeInit(ADC_HandleTypeDef *adcHandle) {

    if (adcHandle->Instance == ADC1) {
        /* USER CODE BEGIN ADC1_MspDeInit 0 */

        /* USER CODE END ADC1_MspDeInit 0 */
        /* Peripheral clock disable */
        __HAL_RCC_ADC_CLK_DISABLE();

        /**ADC1 GPIO Configuration
            PA1     ------> ADC1_IN1.....Picked 1
            PA2     ------> ADC1_IN2.....Picked 0
            PA4     ------> ADC1_IN4.....Tool Filament sensor
            PA5     ------> ADC1_IN5.....NTC 1
            PA7     ------> ADC1_IN7.....NTC internal
            PB0     ------> ADC1_IN8.....Heater current
            PB2     ------> ADC1_IN10....NTC 2
            PB10     ------> ADC1_IN11...Meas 24 V
            */

        HAL_GPIO_DeInit(GPIOA, PICKED1_Pin | PICKED0_Pin | TFS_Pin | NTC_Pin | NTC_INTERNAL_Pin);

        HAL_GPIO_DeInit(GPIOB, HEATER_CURRENT_Pin | NTC2_Pin | MEAS_24V_Pin);

        // ADC1 DMA DeInit
        HAL_DMA_DeInit(adcHandle->DMA_Handle);
    }
}

void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi) {
    GPIO_InitTypeDef GPIO_InitStruct = {};
    if (hspi->Instance == SPI2) {
        /* USER CODE BEGIN SPI2_MspInit 0 */

        /* USER CODE END SPI2_MspInit 0 */
        /* Peripheral clock enable */
        __HAL_RCC_SPI2_CLK_ENABLE();

        __HAL_RCC_GPIOB_CLK_ENABLE();
        /**SPI2 GPIO Configuration
    PB13     ------> SPI2_SCK
    PB14     ------> SPI2_MISO
    PB15     ------> SPI2_MOSI
    */
        GPIO_InitStruct.Pin = GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
        GPIO_InitStruct.Alternate = GPIO_AF0_SPI2;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

        /* USER CODE BEGIN SPI2_MspInit 1 */

        /* USER CODE END SPI2_MspInit 1 */
    }
}

void HAL_SPI_MspDeInit(SPI_HandleTypeDef *hspi) {
    if (hspi->Instance == SPI2) {
        /* USER CODE BEGIN SPI2_MspDeInit 0 */

        /* USER CODE END SPI2_MspDeInit 0 */
        /* Peripheral clock disable */
        __HAL_RCC_SPI2_CLK_DISABLE();

        /**SPI2 GPIO Configuration
    PB13     ------> SPI2_SCK
    PB14     ------> SPI2_MISO
    PB15     ------> SPI2_MOSI
    */
        HAL_GPIO_DeInit(GPIOB, GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15);

        /* USER CODE BEGIN SPI2_MspDeInit 1 */

        /* USER CODE END SPI2_MspDeInit 1 */
    }
}
