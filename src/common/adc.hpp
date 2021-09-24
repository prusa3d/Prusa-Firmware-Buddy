#pragma once

#include "inttypes.h"
#include "stm32f4xx_hal.h"
#include "config_buddy_2209_02.h"
#include "main.h"

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

using AdcDma1 = AdcDma<hadc1, 5>;

extern AdcDma1 adcDma1;

class AdcGet {
public:
    static uint16_t nozzle() { return adcDma1.m_data[0]; };
    static uint16_t bed() { return adcDma1.m_data[1]; };
    static uint16_t boardTemp() { return adcDma1.m_data[2]; };
    static uint16_t pinda() { return adcDma1.m_data[3]; };
    static uint16_t bedMon() { return adcDma1.m_data[4]; };
};

#ifdef SIM_HEATER
class AdcSet {
public:
    static void nozzle(uint16_t value) { adcDma1.m_data[0] = value; };
    static void bed(uint16_t value) { adcDma1.m_data[1] = value; };
    static void temp2(uint16_t value) { adcDma1.m_data[2] = value; };
    static void pinda(uint16_t value) { adcDma1.m_data[3] = value; };
    static void bedMon(uint16_t value) { adcDma1.m_data[4] = value; };
};
#endif
