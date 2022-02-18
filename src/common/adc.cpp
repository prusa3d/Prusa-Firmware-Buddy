#include "adc.hpp"

AdcDma1 adcDma1;
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

void config_adc_ch(ADC_HandleTypeDef *hadc, uint32_t Channel, uint32_t Rank) {
    Rank++; // Channel rank starts at 1, but for array indexing, we need to start from 0.
    ADC_ChannelConfTypeDef sConfig = { Channel, Rank, ADC_SAMPLETIME_480CYCLES, 0 };
    if (HAL_ADC_ConfigChannel(hadc, &sConfig) != HAL_OK) {
        Error_Handler();
    }
}

void config_adc(ADC_HandleTypeDef *hadc, ADC_TypeDef *ADC_NUM, uint32_t NbrOfConversion) {
    hadc->Instance = ADC_NUM;
    hadc->Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV8;
    hadc->Init.Resolution = ADC_RESOLUTION_10B;
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

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
void MX_ADC1_Init(void) {

    /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)*/
    config_adc(&hadc1, ADC1, AdcChannel::ADC1_CH_CNT);
    /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.*/
    config_adc_ch(&hadc1, ADC_CHANNEL_10, AdcChannel::hotend_T);
    config_adc_ch(&hadc1, ADC_CHANNEL_4, AdcChannel::heatbed_T);
    config_adc_ch(&hadc1, ADC_CHANNEL_5, AdcChannel::board_T);
    config_adc_ch(&hadc1, ADC_CHANNEL_6, AdcChannel::pinda_T);
    config_adc_ch(&hadc1, ADC_CHANNEL_3, AdcChannel::heatbed_U);

    HAL_NVIC_DisableIRQ(DMA2_Stream0_IRQn); //Disable ADC DMA IRQ. This IRQ is not used. Save CPU usage.
}
