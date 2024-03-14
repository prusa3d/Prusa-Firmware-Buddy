#pragma once

#include "PuppyConfig.hpp"
#include "IIR1Filter.hpp"
#include <cstdint>
#include <string.h>
#include <modular_bed_errors.hpp>

namespace modularbed {

#define THERMISTOR_FILTER_COEFFICIENT 0.1f
#define THERMISTOR_INITIAL_RESISTANCE 100000.0f

enum class HeatbedletState {
    Inactive, // target temp. = 0
    Heating, // target temp. > 0; measured temp. < target temp.; temperature not stabilized
    Cooling, // target temp. > 0; measured temp. > target temp.; temperature not stabilized
    Stable, // target temp. > 0, temperature stabilized
    Error,
};

using namespace modular_bed_shared::errors;

#if CONTROLLER_TEMP_FILTER_LENGTH > CONTROLLER_TEMP_DIFF_FILTER_LENGTH
    #error "Constant CONTROLLER_TEMP_FILTER_LENGTH must not be greater than constant CONTROLLER_TEMP_DIFF_FILTER_LENGTH."
#endif

struct HeatbedletInfo {
    static const int TEMP_HISTORY_SIZE = CONTROLLER_TEMP_DIFF_FILTER_LENGTH;

    static float m_ChamberTemperature;

    uint32_t m_HBIndex;
    HeatbedletState m_State;
    uint16_t m_ErrorFlags;
    float m_ReferenceResistance;
    float m_ReferenceMaxCurrent;
    float m_MaxAllowedCurrent;
    bool m_MeasureResistance;

    IIR1Filter m_FilteredThermistorResistance;
    float m_ThermistorCalibrationCoef;

    float m_PWMValue;
    float m_RoundedPWMValue;
    bool m_IsPWMLowerSaturated;
    bool m_IsPWMUpperSaturated;
    uint32_t m_PWMPulseLength;

    float m_TargetTemperature;
    float m_MeasuredTemperature;
    float m_MeasuredTemperatureHistory[TEMP_HISTORY_SIZE];
    int m_MeasuredTemperatureHistoryIndex;
    float m_FilteredMeasuredTemperature;
    float m_FilteredMeasuredTemperatureDiff;

    float m_PID_P_Coef;
    float m_PID_I_Coef;
    float m_PID_D_Coef;

    bool m_PID_IsON = false;
    float m_PID_Err;
    float m_PID_P_Action;
    double m_PID_I_Action;
    double m_PID_I_ActionDelta;
    float m_PID_D_Action;

    bool m_TemperatureStable_Detected;
    uint32_t m_TemperatureStable_StartTime;

    bool m_TemperatureDrop_Detected;
    uint32_t m_TemperatureDrop_StartTime;

    bool m_TemperaturePeak_Detected;
    uint32_t m_TemperaturePeak_StartTime;

    uint32_t m_NextPreheatCheckTime;
    uint32_t m_NextPreheatCheckTemperature;

    static HeatbedletInfo *Get(uint32_t heatbedletIndex) {
        return &(s_HBInfoArray[heatbedletIndex]);
    }

    bool IsTempSensorOK();
    bool IsHeaterOK();

    void AddMeasuredTemperature(float temperature);

private:
    HeatbedletInfo(uint32_t hbIndex);

    static HeatbedletInfo s_HBInfoArray[HEATBEDLET_COUNT];
};

} // namespace modularbed
