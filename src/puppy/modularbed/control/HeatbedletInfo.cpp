#include "HeatbedletInfo.hpp"
#include "cmsis_os.h"
#include <cmath>

namespace modularbed {

float HeatbedletInfo::m_ChamberTemperature = ((float)DEFAULT_CHAMBER_TEMP);

HeatbedletInfo HeatbedletInfo::s_HBInfoArray[HEATBEDLET_COUNT] = {
    HeatbedletInfo(0),
    HeatbedletInfo(1),
    HeatbedletInfo(2),
    HeatbedletInfo(3),
    HeatbedletInfo(4),
    HeatbedletInfo(5),
    HeatbedletInfo(6),
    HeatbedletInfo(7),
    HeatbedletInfo(8),
    HeatbedletInfo(9),
    HeatbedletInfo(10),
    HeatbedletInfo(11),
    HeatbedletInfo(12),
    HeatbedletInfo(13),
    HeatbedletInfo(14),
    HeatbedletInfo(15),
};

HeatbedletInfo::HeatbedletInfo(uint32_t hbIndex)
    : m_HBIndex(hbIndex)
    , m_State(HeatbedletState::Inactive)
    , m_ErrorFlags(0)
    , m_ReferenceResistance(HEATBEDLET_DEFAULT_RESISTANCE)
    , m_ReferenceMaxCurrent(((float)HEATBEDLET_VOLTAGE) / HEATBEDLET_DEFAULT_RESISTANCE)
    , m_MaxAllowedCurrent(HEATBEDLET_DEFAULT_MAX_ALLOWED_CURRENT)
    , m_MeasureResistance(true)
    , m_FilteredThermistorResistance(THERMISTOR_FILTER_COEFFICIENT, THERMISTOR_INITIAL_RESISTANCE)
    , m_ThermistorCalibrationCoef(1)
    , m_PWMValue(0)
    , m_RoundedPWMValue(0)
    , m_IsPWMLowerSaturated(false)
    , m_IsPWMUpperSaturated(false)
    , m_PWMPulseLength(0)
    , m_TargetTemperature(0)
    , m_MeasuredTemperature(0)
    , m_MeasuredTemperatureHistoryIndex(0)
    , m_FilteredMeasuredTemperature(0)
    , m_FilteredMeasuredTemperatureDiff(0)
    , m_PID_P_Coef(0)
    , m_PID_I_Coef(0)
    , m_PID_D_Coef(0)
    , m_PID_IsON(false)
    , m_PID_Err(0)
    , m_PID_P_Action(0)
    , m_PID_I_Action(0)
    , m_PID_I_ActionDelta(0)
    , m_PID_D_Action(0)
    , m_TemperatureStable_Detected(false)
    , m_TemperatureStable_StartTime(0)
    , m_TemperatureDrop_Detected(false)
    , m_TemperatureDrop_StartTime(0)
    , m_TemperaturePeak_Detected(false)
    , m_TemperaturePeak_StartTime(0)
    , m_NextPreheatCheckTime(0)
    , m_NextPreheatCheckTemperature(0) {

    for (int i = 0; i < TEMP_HISTORY_SIZE; i++) {
        m_MeasuredTemperatureHistory[i] = 0;
    }
};

bool HeatbedletInfo::IsTempSensorOK() {
    return 0 == (m_ErrorFlags & (((uint16_t)HeatbedletError::TemperatureBelowMinimum) | ((uint16_t)HeatbedletError::TemperatureAboveMaximum)));
}

bool HeatbedletInfo::IsHeaterOK() {
    return 0 == (m_ErrorFlags & (((uint16_t)HeatbedletError::HeaterDisconnected) | ((uint16_t)HeatbedletError::HeaterShortCircuit)));
}

void HeatbedletInfo::AddMeasuredTemperature(float temperature) {
    m_MeasuredTemperature = temperature;

    // calculate moving average of temperature differential
    float diff = temperature - m_MeasuredTemperatureHistory[m_MeasuredTemperatureHistoryIndex];
    float coef = ((float)CONTROLLER_TEMP_DIFF_FILTER_LENGTH) / (1000.0f / ADC_CONVERSION_PERIOD / ADC_BATCH_SIZE);
    m_FilteredMeasuredTemperatureDiff = diff / coef;

    // calculate moving average of temperature
    m_MeasuredTemperatureHistory[m_MeasuredTemperatureHistoryIndex] = temperature;
    float sum = 0;
    for (int i = 0; i < CONTROLLER_TEMP_FILTER_LENGTH; i++) {
        int idx = m_MeasuredTemperatureHistoryIndex - i;
        if (idx < 0) {
            idx += TEMP_HISTORY_SIZE;
        }
        sum += m_MeasuredTemperatureHistory[idx];
    }
    m_FilteredMeasuredTemperature = sum / CONTROLLER_TEMP_FILTER_LENGTH;

    // shift index of the circular buffer
    m_MeasuredTemperatureHistoryIndex++;
    if (m_MeasuredTemperatureHistoryIndex == TEMP_HISTORY_SIZE) {
        m_MeasuredTemperatureHistoryIndex = 0;
    }
}

} // namespace modularbed
