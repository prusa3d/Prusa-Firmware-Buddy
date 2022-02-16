#pragma once

#include "inttypes.h"
#include "stm32f4xx_hal.h"
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
};

void MX_ADC1_Init(void);
extern ADC_HandleTypeDef hadc1;

template <ADC_HandleTypeDef &adc, size_t channels>
class AdcDma {
public:
    AdcDma()
        : m_data() {};
    void init() {
#ifndef SIM_HEATER
        HAL_ADC_Init(&adc);
        HAL_ADC_Start_DMA(&adc, reinterpret_cast<uint32_t *>(m_data), channels); // Start ADC in DMA mode and
#else
        m_data[0] = ADC_SIM_VAL4;
        m_data[1] = ADC_SIM_VAL1;
#endif
    };
    void deinit() {
        HAL_ADC_Stop_DMA(&adc);
        HAL_ADC_DeInit(&adc);
    }

private:
    uint16_t m_data[channels];
    friend class AdcGet;
#ifdef SIM_HEATER
    friend class AdcSet;
#endif
};

using AdcDma1 = AdcDma<hadc1, AdcChannel::ADC1_CH_CNT>;

extern AdcDma1 adcDma1;

class AdcGet {
public:
    static uint16_t nozzle() { return adcDma1.m_data[AdcChannel::hotend_T]; };
    static uint16_t bed() { return adcDma1.m_data[AdcChannel::heatbed_T]; };
    static uint16_t boardTemp() { return adcDma1.m_data[AdcChannel::board_T]; };
    static uint16_t pinda() { return adcDma1.m_data[AdcChannel::pinda_T]; };
    static uint16_t bedMon() { return adcDma1.m_data[AdcChannel::heatbed_U]; };
};

#ifdef SIM_HEATER
class AdcSet {
public:
    static void nozzle(uint16_t value) { adcDma1.m_data[AdcChannel::hotend_T] = value; };
    static void bed(uint16_t value) { adcDma1.m_data[AdcChannel::heatbed_T] = value; };
    static void temp2(uint16_t value) { adcDma1.m_data[AdcChannel::board_T] = value; };
    static void pinda(uint16_t value) { adcDma1.m_data[AdcChannel::pinda_T] = value; };
    static void bedMon(uint16_t value) { adcDma1.m_data[AdcChannel::heatbed_U] = value; };
};
#endif
