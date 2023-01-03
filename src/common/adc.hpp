#pragma once

#include "inttypes.h"
#include <device/hal.h>
#include <device/board.h>
#include "config_buddy_2209_02.h"
#include "main.h"

/*
ADC channels, ranks, ...
Buddy board:
    ADC1:
        PC0  - Rank 1 - CH10 - therm 0(hotend temp)
        PA4  - Rank 2 - CH4  - therm 1(heatbed temp)
        PA5  - Rank 3 - CH5  - therm 2(board temp)
        PA6  - Rank 4 - CH6  - therm pinda
        PA3  - Rank 5 - CH3  - heatbed voltage
*/

namespace AdcChannel {
enum AD1 { //ADC1 channels
    hotend_T,
    heatbed_T,
    board_T,
    pinda_T,
    heatbed_U,
    ADC1_CH_CNT
};
}

template <ADC_HandleTypeDef &adc, size_t channels>
class AdcDma {
public:
    AdcDma()
        : m_data() {};
    void init() {
        HAL_ADC_Init(&adc);
        HAL_ADC_Start_DMA(&adc, reinterpret_cast<uint32_t *>(m_data), channels); // Start ADC in DMA mode and
    };

    void deinit() {
        HAL_ADC_Stop_DMA(&adc);
        HAL_ADC_DeInit(&adc);
    }

    [[nodiscard]] uint16_t get_channel(uint8_t index) const {
        return m_data[index];
    }

private:
    uint16_t m_data[channels];
};

using AdcDma1 = AdcDma<hadc1, AdcChannel::ADC1_CH_CNT>;
extern AdcDma1 adcDma1;

namespace AdcGet {
[[nodiscard]] inline uint16_t get(uint8_t channel) { return adcDma1.get_channel(channel); }
inline uint16_t nozzle() { return get(AdcChannel::hotend_T); }
inline uint16_t bed() { return get(AdcChannel::heatbed_T); }
inline uint16_t boardTemp() { return get(AdcChannel::board_T); }
inline uint16_t pinda() { return get(AdcChannel::pinda_T); }
inline uint16_t bedMon() { return get(AdcChannel::heatbed_U); }
}
