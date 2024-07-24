#include "MeasurementCalculator.hpp"
#include "PuppyConfig.hpp"
#include <cmath>

#define KELVIN_SHIFT 273.15f

namespace modularbed {

float CalcThermistorResistance([[maybe_unused]] hal::ADCDriver::ADCChannel channel, uint16_t adcValue) {
    float voltage = ((float)adcValue) / 0x10000;
    float tmp = voltage / (1 - voltage);
    float resistance = tmp * ((float)HEATBEDLET_THERMISTOR_VOLTAGE_DIVIDER_RESISTANCE);
    return resistance;
}

float CalcThermistorTemperature(float resistance) {
    float tmp;
    tmp = resistance / ((float)HEATBEDLET_THERMISTOR_RESISTANCE_AT_T_ZERO);
    tmp = logf(tmp);
    tmp = tmp / ((float)HEATBEDLET_THERMISTOR_B_CONSTANT);
    tmp = tmp + 1.0f / (((float)HEATBEDLET_THERMISTOR_T_ZERO) + KELVIN_SHIFT);
    tmp = 1.0f / tmp;
    tmp = tmp - KELVIN_SHIFT;

    return tmp;
}

float CalcElectricCurrent(int adcValue) {
    // This calculation takes an average of 39 microseconds on STM32G0 @56MHz

    float voltage = ((float)adcValue) * ADC_REFERENCE_VOLTAGE / ((float)0x10000);

    float current = voltage * ADC_REFERENCE_VOLTAGE / CURRENT_SENSOR_REFERENCE_VOLTAGE / ((float)CURRENT_SENSOR_SENSITIVITY_MILLIVOLTS_PER_AMPERE) * 1000;

    return current;
}

float CalcHBReferenceResistance(float current, float temperature) {
    // safeguard just in case to avoid division by 0
    if (current != 0) {
        float resistance = ((float)HEATBEDLET_VOLTAGE) / current;
        resistance = resistance / (1 + HEATBEDLET_RESISTANCE_TEMPERATURE_COEFFICIENT * (temperature - HEATBEDLET_REFERENCE_TEMPERATURE));
        return resistance;
    }
    return INF_RESISTANCE;
}

float CalcHBResistanceAtTemperature(float referenceResistance, float temperature) {
    float resistance = referenceResistance * (1 + HEATBEDLET_RESISTANCE_TEMPERATURE_COEFFICIENT * (temperature - HEATBEDLET_REFERENCE_TEMPERATURE));
    return resistance;
}

} // namespace modularbed
