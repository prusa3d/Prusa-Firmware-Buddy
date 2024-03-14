#include "PuppyConfig.hpp"
#include "hal/HAL_ADC.hpp"
#include "hal/HAL_System.hpp"
#include "hal/HAL_Common.hpp"

#include "stm32g0xx_hal.h"
#include <cstring>

#define ADC_INSTANCE            ADC1
#define ADC_COMMON_INSTANCE     ADC1_COMMON
#define ADC_BIT_WAITING_TIMEOUT 500 // measured maximum value on STM32G0@56Mhz is 111
namespace {
constexpr float ADC_CALIBRATION_VOLTAGE = 3.0f;
}

/*
    ** ADC speed and precision **
    Clock source is set to PLLADC, which is configured to run at 10MHz in HAL_System.cpp. There is also additional prescaler configured in CCR register.
    Effective ADC clock speed is 5MHz, sampling time is 160.5 clock cycles and oversampling ratio is 64.
    This assures high measurement precision at the cost of low speed. One ADC measurement takes 2220 microseconds.


    ** Heatbedlet measurement order **
    Expected and optimal heatbedlet temperature measurement order is this: 0, 1, 2, 3, 4, 5, 12, 14, 6, 7, 8, 9, 10, 11, 13, 15.
    When temperatures are measured in this order, then MUX is switched just twice per measurement period.
    There is also maximum possible time (half of the period) for MUX stabilization.
*/

namespace hal::ADCDriver {

static ADCChannel m_ActualChannel = ADCChannel::Heatbedlet_0;

static uint32_t TranslateADCChannel(ADCChannel channel);
static void SetMux(ADCChannel channel);
static void WaitForBit(volatile uint32_t *pRegister, uint32_t bitMask, bool expectedValue);

bool Init() {
    // Initializes the peripherals clocks
    RCC_PeriphCLKInitTypeDef PeriphClkInit {};
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
    PeriphClkInit.AdcClockSelection = RCC_ADCCLKSOURCE_PLLADC;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
        Error_Handler();
    }

    // Peripheral clock enable
    __HAL_RCC_ADC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    // init analog input pins
    GPIO_InitTypeDef GPIO_InitStruct {};
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_4 | GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // init MUX switching pins
    GPIO_InitStruct.Pin = GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    // Disable all interrupts
    CLEAR_BIT(ADC_INSTANCE->IER, ADC_IER_ADRDYIE | ADC_IER_EOSMPIE | ADC_IER_EOCIE | ADC_IER_EOSIE | ADC_IER_OVRIE | ADC_IER_AWD1IE | ADC_IER_AWD2IE | ADC_IER_AWD3IE | ADC_IER_EOCALIE | ADC_IER_CCRDYIE);

    // Enable internal ADC voltage regulator
    LL_ADC_EnableInternalRegulator(ADC_INSTANCE);

    // Cofigure oversampling ratio: 64 measurements
    SET_BIT(ADC_INSTANCE->CFGR2, ADC_CFGR2_OVSE | ADC_CFGR2_OVSR_0 | ADC_CFGR2_OVSR_2);

    // Cofigure oversampling shift: 2 bits
    SET_BIT(ADC_INSTANCE->CFGR2, ADC_CFGR2_OVSS_1);

    // Configure sampling time: 160.5 clock cycles
    SET_BIT(ADC_INSTANCE->SMPR, ADC_SMPR_SMP1_0 | ADC_SMPR_SMP1_1 | ADC_SMPR_SMP1_2 | ADC_SMPR_SMP2_0 | ADC_SMPR_SMP2_1 | ADC_SMPR_SMP2_2);

    // Configure ADC clock prescaler to 2
    SET_BIT(ADC_COMMON_INSTANCE->CCR, ADC_CCR_PRESC_0);

    // Run automatic self-calibration
    WaitForBit(&ADC_INSTANCE->ISR, ADC_ISR_CCRDY, true);
    SET_BIT(ADC_INSTANCE->CR, ADC_CR_ADCAL);
    WaitForBit(&ADC_INSTANCE->CR, ADC_CR_ADCAL, false);

    // Enable temperature sensor
    SET_BIT(ADC_COMMON_INSTANCE->CCR, ADC_CCR_TSEN);

    // Enable reference voltage channel
    SET_BIT(ADC_COMMON_INSTANCE->CCR, ADC_CCR_VREFEN);

    // Enable ADC device
    SET_BIT(ADC_INSTANCE->CR, ADC_CR_ADEN);

    SetMux(ADCChannel::Heatbedlet_6);

    return true;
}

void PrepareConversion(ADCChannel channel) {
    SetMux(channel);
    uint32_t adcChannel = TranslateADCChannel(channel);

    // Select ADC channel
    ADC_INSTANCE->CHSELR = 1 << adcChannel;

    WaitForBit(&ADC_INSTANCE->ISR, ADC_ISR_CCRDY, true);
}

void StartConversion(ADCChannel channel) {
    // ADC conversion takes 2220 microseconds on STM32G0 @56MHz with current ADC configuration

    PrepareConversion(channel);

    // Start ADC conversion
    SET_BIT(ADC_INSTANCE->CR, ADC_CR_ADSTART);

    m_ActualChannel = channel;
}

bool CancelConversion() {
    SET_BIT(ADC_INSTANCE->CR, ADC_CR_ADSTP);
    WaitForBit(&ADC_INSTANCE->CR, ADC_CR_ADSTP, false);

    return true;
}

bool IsConversionFinished() {
    bool running = READ_BIT(ADC_INSTANCE->CR, ADC_CR_ADSTART) != 0;
    return !running;
}

ADCChannel GetConvertedChannel() {
    return m_ActualChannel;
}

uint16_t GetConversionResult() {
    uint32_t value = ADC_INSTANCE->DR;
    return value;
}

float CalculateMCUTemperature(float TEMP_ADCValue, uint32_t VREF_ADCValue) {
    static constexpr float temp_diff = static_cast<uint16_t>(TEMPSENSOR_CAL2_TEMP) - static_cast<uint16_t>(TEMPSENSOR_CAL1_TEMP);
    static const float ta = temp_diff / (static_cast<uint16_t>(*TEMPSENSOR_CAL2_ADDR) - static_cast<uint16_t>(*TEMPSENSOR_CAL1_ADDR));
    static const float tb = static_cast<uint16_t>(TEMPSENSOR_CAL1_TEMP) - ta * static_cast<uint16_t>(*TEMPSENSOR_CAL1_ADDR);

    float vref_coef = (static_cast<float>(*VREFINT_CAL_ADDR) / (VREF_ADCValue / 16.0f));

    return ta * ((TEMP_ADCValue) / 16 * vref_coef) + tb;
}

float CalculateVREF(uint32_t VREF_ADCValue) {
    return ((static_cast<float>(*VREFINT_CAL_ADDR) / (VREF_ADCValue / 16)) * ADC_CALIBRATION_VOLTAGE);
}

uint32_t TranslateADCChannel(ADCChannel channel) {
    switch (channel) {
    case ADCChannel::Heatbedlet_8: // 0
        return 16;
    case ADCChannel::Heatbedlet_9: // 1
        return 15;
    case ADCChannel::Heatbedlet_10: // 2
        return 4;
    case ADCChannel::Heatbedlet_11: // 3
        return 3;
    case ADCChannel::Heatbedlet_12: // 4
        return 2;
    case ADCChannel::Heatbedlet_13: // 5
        return 0;
    case ADCChannel::Heatbedlet_14: // 6
        return 1;
    case ADCChannel::Heatbedlet_15: // 7
        return 5;
    case ADCChannel::Heatbedlet_0: // 8
        return 7;
    case ADCChannel::Heatbedlet_1: // 9
        return 17;
    case ADCChannel::Heatbedlet_2: // 10
        return 18;
    case ADCChannel::Heatbedlet_3: // 11
        return 8;
    case ADCChannel::Heatbedlet_4: // 12
        return 9;
    case ADCChannel::Heatbedlet_5: // 13
        return 9;
    case ADCChannel::Heatbedlet_6: // 14
        return 11;
    case ADCChannel::Heatbedlet_7: // 15
        return 11;
    case ADCChannel::Current_A:
        return 6;
    case ADCChannel::Current_B:
        return 10;
    case ADCChannel::MCUTemperature:
        return 12;
    case ADCChannel::VREF:
        return 13;
    default:
        assert(false && "should not happen");
        return 16;
    }
}

void SetMux(ADCChannel channel) {
    switch (channel) {
    case ADCChannel::Heatbedlet_14: // set MUX earlier, so current will be stablized
    case ADCChannel::Heatbedlet_5:
    case ADCChannel::Heatbedlet_7:
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_SET);
        break;
    case ADCChannel::Heatbedlet_8: // set MUX earlier, so current will be stablized
    case ADCChannel::Heatbedlet_4:
    case ADCChannel::Heatbedlet_6:
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_RESET);
        break;
    default:
        break;
    }
}

void WaitForBit(volatile uint32_t *pRegister, uint32_t bitMask, bool expectedValue) {
    uint32_t max_loop_count = ADC_BIT_WAITING_TIMEOUT;
    uint32_t expected = expectedValue ? bitMask : 0;
    while (((*pRegister) & bitMask) != expected && max_loop_count > 0) {
        max_loop_count--;
    }
}

} // namespace hal::ADCDriver
