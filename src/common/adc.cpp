#include "adc.h"
#include <array>
#include "tuple"

using channels_t = std::array<uint16_t, 5>;
using peripheral_index_t = std::integral_constant<uint32_t, 0>;
using channels_index_t = std::integral_constant<uint32_t, 1>;
using adc_t = std::tuple<ADC_HandleTypeDef *, channels_t>;
using adc_list_t = std::array<adc_t, 2>;

static adc_list_t g_adcs { { { nullptr, { { 0x00 } } } } };

void adc_dma_init(ADC_HandleTypeDef *adc) {
    if (adc == nullptr) {
        return;
    }

    for (auto &_adcs : g_adcs) {
        if (std::get<peripheral_index_t::value>(_adcs) == nullptr) {
#ifndef SIM_HEATER
            HAL_ADC_Init(adc);
            HAL_ADC_Start_DMA(adc, (uint32_t *)std::get<channels_index_t::value>(_adcs).data(), std::get<channels_index_t::value>(_adcs).size()); // Start ADC in DMA mode and
#else
            std::get<channels_index_t::value>(_adcs)[CHANNEL_BED] = ADC_SIM_VAL1;
            std::get<channels_index_t::value>(_adcs)[CHANNEL_NOZZLE] = ADC_SIM_VAL4;
#endif
            std::get<peripheral_index_t::value>(_adcs) = adc;
        }
    }
}

void adc_dma_deinit(ADC_HandleTypeDef *adc) {
    if (adc == nullptr) {
        return;
    }

    for (auto &_adcs : g_adcs) {

        if (adc == std::get<peripheral_index_t::value>(_adcs)) {
#ifndef SIM_HEATER
            HAL_ADC_Stop_DMA(adc);
            HAL_ADC_DeInit(adc);
#else
            std::get<channels_index_t::value>(_adcs)[CHANNEL_BED] = 0;
            std::get<channels_index_t::value>(_adcs)[CHANNEL_NOZZLE] = 0;
#endif
            std::get<peripheral_index_t::value>(_adcs) = nullptr;
        }
    }
}

uint16_t get_adc_channel_value(ADC_HandleTypeDef *adc, uint32_t channel) {
    for (auto &_adcs : g_adcs) {
        if (std::get<peripheral_index_t::value>(_adcs) == adc && channel < std::get<channels_index_t::value>(_adcs).size()) {
            return std::get<channels_index_t::value>(_adcs)[channel];
        }
    }
    return UINT16_MAX;
}

#ifdef SIM_HEATER

void set_adc_channel_value(ADC_HandleTypeDef *adc, uint32_t channel, uint16_t value) {
    for (auto &_adcs : g_adcs) {
        if (std::get<peripheral_index_t::value>(_adcs) == adc && channel < std::get<channels_index_t::value>(_adcs).size()) {
            std::get<channels_index_t::value>(_adcs)[channel] = value;
        }
    }
}
#endif
