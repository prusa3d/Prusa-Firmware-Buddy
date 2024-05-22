#pragma once

#include <cstdint>

namespace hal::ADCDriver {

enum class ADCChannel {
    Heatbedlet_0 = 0,
    Heatbedlet_1,
    Heatbedlet_2,
    Heatbedlet_3,
    Heatbedlet_4,
    Heatbedlet_5,
    Heatbedlet_6,
    Heatbedlet_7,
    Heatbedlet_8,
    Heatbedlet_9,
    Heatbedlet_10,
    Heatbedlet_11,
    Heatbedlet_12,
    Heatbedlet_13,
    Heatbedlet_14,
    Heatbedlet_15,
    Current_A,
    Current_B,
    MCUTemperature,
    VREF,
    CHANNEL_COUNT,
};

bool Init();

void PrepareConversion(ADCChannel channel);
void StartConversion(ADCChannel channel);
bool CancelConversion();
bool IsConversionFinished();
ADCChannel GetConvertedChannel();
uint16_t GetConversionResult();
float CalculateMCUTemperature(float TEMP_ADCValue, uint32_t VREF_ADCValue); // calculation is hardware dependent
float CalculateVREF(uint32_t VREF_ADCValue); // calculation is hardware dependent

} // namespace hal::ADCDriver
