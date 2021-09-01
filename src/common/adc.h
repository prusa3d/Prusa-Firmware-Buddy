#pragma once

#include "inttypes.h"
#include "stm32f4xx_hal.h"
#include "config_buddy_2209_02.h"

const uint8_t CHANNEL_NOZZLE = 0;
const uint8_t CHANNEL_BED = 1;
const uint8_t CHANNEL_TEMP_2 = 2;
const uint8_t CHANNEL_PINDA = 3;
const uint8_t CHANNEL_BED_MON = 4;

extern void adc_dma_init(ADC_HandleTypeDef *adc);
extern void adc_dma_deinit(ADC_HandleTypeDef *adc);
extern uint16_t get_adc_channel_value(ADC_HandleTypeDef *adc, uint32_t channel);

#ifdef SIM_HEATER
extern void set_adc_channel_value(ADC_HandleTypeDef *adc, uint32_t channel, uint16_t value);
#endif
