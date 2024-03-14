#include "StateLogic.hpp"
#include "PWMLogic.hpp"
#include "cmsis_os.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <cstdint>

using namespace modularbed::ModbusRegisters;
using namespace hal::ADCDriver;

#define EXPECTED_CURRENT_FILTER_MULTIPLIER 1000

namespace modularbed::StateLogic {

uint16_t s_SystemErrorFlags = 0;

bool s_IsPowerPanicActive = false;
bool s_IsCurrentFaultActive = false;

std::array<MovingAverageFilter, 2> s_ExpectedCurrentFilter({
    CURRENT_FILTER_DATA_POINT_COUNT,
    CURRENT_FILTER_DATA_POINT_COUNT,
});

void SetState(uint32_t heatbedletIndex, HeatbedletState state);
void SetSystemError(SystemError error);
void CollectSystemErrorsFromHB();

void CheckMinMaxTemperature(uint32_t heatbedletIndex);
void CheckTemperatureStabilized(uint32_t heatbedletIndex);
void CheckTemperatureDrop(uint32_t heatbedletIndex);
void CheckTemperaturePeak(uint32_t heatbedletIndex);
void StartPreheatWatching(uint32_t heatbedletIndex);
void CheckPreheatError(uint32_t heatbedletIndex);

void ProcessMCUTemperature(float temperature);

void ProcessMeasuredValue(ADCChannel channel, float value) {
    switch (channel) {
    case ADCChannel::Heatbedlet_0:
    case ADCChannel::Heatbedlet_1:
    case ADCChannel::Heatbedlet_2:
    case ADCChannel::Heatbedlet_3:
    case ADCChannel::Heatbedlet_4:
    case ADCChannel::Heatbedlet_5:
    case ADCChannel::Heatbedlet_6:
    case ADCChannel::Heatbedlet_7:
    case ADCChannel::Heatbedlet_8:
    case ADCChannel::Heatbedlet_9:
    case ADCChannel::Heatbedlet_10:
    case ADCChannel::Heatbedlet_11:
    case ADCChannel::Heatbedlet_12:
    case ADCChannel::Heatbedlet_13:
    case ADCChannel::Heatbedlet_14:
    case ADCChannel::Heatbedlet_15:
        ProcessMeasuredHeatbedletTemperature(channel, value);
        break;
    case ADCChannel::Current_A:
    case ADCChannel::Current_B:
        ProcessMeasuredElectricCurrent(channel, value);
        break;
    case ADCChannel::VREF:
        break;
    case ADCChannel::MCUTemperature:
        ProcessMCUTemperature(value);
        break;
    case ADCChannel::CHANNEL_COUNT:
        // do nothing
        break;
    }
}

void ProcessMeasuredHeatbedletTemperature(ADCChannel channel, float temperature) {
    float temperatureModbus = temperature * MODBUS_TEMPERATURE_REGISTERS_SCALE + 0.5f;
    if (temperatureModbus < 0) {
        temperatureModbus = 0;
    } else if (temperatureModbus > 0xFFFF) {
        temperatureModbus = 0xFFFF;
    }

    uint16_t heatbedletIndex = ((uint16_t)channel) - ((uint16_t)ADCChannel::Heatbedlet_0);

    SetRegValue(HBInputRegister::measured_temperature, heatbedletIndex, (uint16_t)temperatureModbus);

    HeatbedletInfo::Get(heatbedletIndex)->AddMeasuredTemperature(temperature);

    CheckMinMaxTemperature(heatbedletIndex);
    if (ENABLE_TEMPERATURE_CHECKS) {
        CheckTemperatureStabilized(heatbedletIndex);
        CheckTemperatureDrop(heatbedletIndex);
        CheckTemperaturePeak(heatbedletIndex);
        CheckPreheatError(heatbedletIndex);
    }
}

void ProcessMCUTemperature(float temperature) {
    SetInputRegisterValue(static_cast<uint16_t>(SystemInputRegister::mcu_temperature), static_cast<uint16_t>(temperature));
}

void ProcessMeasuredElectricCurrent(ADCChannel channel, float current) {
    float modbusCurrent = current * MODBUS_CURRENT_REGISTERS_SCALE;
    int16_t adcCurrentRegValue = std::clamp(
        std::lround(modbusCurrent),
        static_cast<long>(std::numeric_limits<int16_t>::min()),
        static_cast<long>(std::numeric_limits<int16_t>::max()));

    const uint8_t idx = channel == ADCChannel::Current_A ? Branch::A : Branch::B;
    const uint32_t intExpectedCurrent = s_ExpectedCurrentFilter[idx].AddValue(static_cast<uint32_t>(PWMLogic::GetExpectedCurrent(idx) * EXPECTED_CURRENT_FILTER_MULTIPLIER));
    const uint16_t expectedCurrentRegValue = MODBUS_CURRENT_REGISTERS_SCALE * intExpectedCurrent / EXPECTED_CURRENT_FILTER_MULTIPLIER;

    SetInputRegisterValue(ftrstd::to_underlying(SystemInputRegister::adc_current_1) + idx, static_cast<uint16_t>(adcCurrentRegValue));
    SetInputRegisterValue(ftrstd::to_underlying(SystemInputRegister::expected_current_1) + idx, static_cast<uint16_t>(expectedCurrentRegValue));

    if (current > OVERCURRENT_THRESHOLD_AMPS[idx]) {
        SetSystemError(SystemError::OverCurrent);
    }

    const float expectedCurrent = static_cast<float>(intExpectedCurrent) / EXPECTED_CURRENT_FILTER_MULTIPLIER;
    if (current > (expectedCurrent + UNEXPECTED_CURRENT_TOLERANCE[idx])) {
        SetSystemError(SystemError::UnexpectedCurrent);
    }
}

void TargetTemperatureChanged(uint32_t heatbedletIndex, float temperatureDegreesC) {
    HeatbedletInfo *pHBInfo = HeatbedletInfo::Get(heatbedletIndex);

    pHBInfo->m_TargetTemperature = temperatureDegreesC;

    if (pHBInfo->IsTempSensorOK() && pHBInfo->IsHeaterOK()) {
        if (temperatureDegreesC <= 0) {
            SetState(heatbedletIndex, HeatbedletState::Inactive);
        } else if (temperatureDegreesC > pHBInfo->m_MeasuredTemperature) {
            SetState(heatbedletIndex, HeatbedletState::Heating);
        } else if (temperatureDegreesC < pHBInfo->m_MeasuredTemperature) {
            SetState(heatbedletIndex, HeatbedletState::Cooling);
        }
    }
}

void SignalsRefreshed(bool panic, bool fault) {
    s_IsPowerPanicActive = panic;
    s_IsCurrentFaultActive = fault;

    SetBitValue(SystemDiscreteInput::power_painc_status, panic);
    SetBitValue(SystemDiscreteInput::current_fault_status, fault);
}

void SetState(uint32_t heatbedletIndex, HeatbedletState state) {
    HeatbedletInfo *pHBInfo = HeatbedletInfo::Get(heatbedletIndex);

    if (pHBInfo->m_State != state) {
        switch (state) {
        case HeatbedletState::Inactive:
            break;
        case HeatbedletState::Heating:
            pHBInfo->m_TemperatureStable_Detected = false;
            StartPreheatWatching(heatbedletIndex);
            break;
        case HeatbedletState::Cooling:
            pHBInfo->m_TemperatureStable_Detected = false;
            break;
        case HeatbedletState::Stable:
            break;
        case HeatbedletState::Error:
            break;
        }

        pHBInfo->m_State = state;
    }
}

void SetHBErrorFlag(uint32_t heatbedletIndex, HeatbedletError error) {
    HeatbedletInfo *pHBInfo = HeatbedletInfo::Get(heatbedletIndex);

    pHBInfo->m_ErrorFlags |= (uint32_t)error;

    SetState(heatbedletIndex, HeatbedletState::Error);

    SetRegValue(HBInputRegister::fault_status, heatbedletIndex, pHBInfo->m_ErrorFlags);
    SetBitValue(HBDiscreteInput::is_ready, heatbedletIndex, false);

    CollectSystemErrorsFromHB();
}

void ClearHBErrorBits(uint32_t heatbedletIndex, uint32_t errorMask) {
    HeatbedletInfo *pHBInfo = HeatbedletInfo::Get(heatbedletIndex);

    pHBInfo->m_ErrorFlags &= ~errorMask;

    SetRegValue(HBInputRegister::fault_status, heatbedletIndex, pHBInfo->m_ErrorFlags);

    if (pHBInfo->m_ErrorFlags == 0) {
        if (pHBInfo->m_State == HeatbedletState::Error) {
            SetState(heatbedletIndex, HeatbedletState::Inactive);
        }
        SetBitValue(HBDiscreteInput::is_ready, heatbedletIndex, true);
    }
}

void SetSystemError(SystemError error) {
    s_SystemErrorFlags |= ((uint16_t)error);

    SetRegValue(SystemInputRegister::fault_status, s_SystemErrorFlags);
}

void CollectSystemErrorsFromHB() {
    uint16_t systemErrors = 0;

    for (int i = 0; i < HEATBEDLET_COUNT; i++) {
        HeatbedletInfo *pHBInfo = HeatbedletInfo::Get(i);
        uint16_t err = pHBInfo->m_ErrorFlags;

        if (err != 0) {
            systemErrors |= (uint16_t)SystemError::HeatbedletError;

            if ((err & ((uint16_t)HeatbedletError::TemperatureBelowMinimum)) != 0) {
                systemErrors |= (uint16_t)SystemError::Mintemp;
            }

            if ((err & ((uint16_t)HeatbedletError::TemperatureAboveMaximum)) != 0) {
                systemErrors |= (uint16_t)SystemError::Maxtemp;
            }

            if ((err & ((uint16_t)HeatbedletError::TemperatureDropDetected)) != 0) {
                systemErrors |= (uint16_t)SystemError::ThermalRunaway;
            }

            if ((err & ((uint16_t)HeatbedletError::TemperaturePeakDetected)) != 0) {
                systemErrors |= (uint16_t)SystemError::ThermalRunaway;
            }

            if ((err & ((uint16_t)HeatbedletError::PreheatError)) != 0) {
                systemErrors |= (uint16_t)SystemError::PreheatError;
            }
        }
    }

    s_SystemErrorFlags |= systemErrors;

    SetRegValue(SystemInputRegister::fault_status, s_SystemErrorFlags);
}

void ClearSystemErrorBits(uint32_t errorMask) {
    s_SystemErrorFlags &= ~errorMask;
    CollectSystemErrorsFromHB();
}

bool IsPowerPanicActive() {
    return s_IsPowerPanicActive;
};

bool IsCurrentFaultActive() {
    return s_IsCurrentFaultActive;
};

bool IsAnyFaultActive() {
    return s_SystemErrorFlags != 0;
};

void CheckMinMaxTemperature(uint32_t heatbedletIndex) {
    HeatbedletInfo *pHBInfo = HeatbedletInfo::Get(heatbedletIndex);
    bool disconnectedError = false;

    if (is_used_bedlet(heatbedletIndex) && (pHBInfo->m_MeasuredTemperature < TEMPERATURE_MIN)) {
        SetHBErrorFlag(heatbedletIndex, HeatbedletError::TemperatureBelowMinimum);
        disconnectedError = true;
    }
    if (!is_used_bedlet(heatbedletIndex) && (pHBInfo->m_MeasuredTemperature >= TEMPERATURE_MIN)) {
        SetHBErrorFlag(heatbedletIndex, HeatbedletError::HeaterConnected);
        disconnectedError = true;
    }

    if (pHBInfo->m_MeasuredTemperature > TEMPERATURE_MAX) {
        SetHBErrorFlag(heatbedletIndex, HeatbedletError::TemperatureAboveMaximum);
        disconnectedError = true;
    }

    if (disconnectedError) {
        pHBInfo->m_MeasureResistance = true;
    }
}

void CheckTemperatureStabilized(uint32_t heatbedletIndex) {
    HeatbedletInfo *pHBInfo = HeatbedletInfo::Get(heatbedletIndex);

    if (pHBInfo->m_State == HeatbedletState::Heating || pHBInfo->m_State == HeatbedletState::Cooling) {
        if (pHBInfo->m_MeasuredTemperature >= (pHBInfo->m_TargetTemperature - TEMPERATURE_STABILIZED_TOLERANCE_DEGREES)
            && pHBInfo->m_MeasuredTemperature <= (pHBInfo->m_TargetTemperature + TEMPERATURE_STABILIZED_TOLERANCE_DEGREES)) {

            if (pHBInfo->m_TemperatureStable_Detected == false) {
                pHBInfo->m_TemperatureStable_Detected = true;
                pHBInfo->m_TemperatureStable_StartTime = osKernelSysTick();
            }

            if (((int32_t)(osKernelSysTick() - pHBInfo->m_TemperatureStable_StartTime)) >= (TEMPERATURE_STABILIZED_MIN_SECONDS * TICKS_PER_SECOND)) {
                SetState(heatbedletIndex, HeatbedletState::Stable);
            }
        } else {
            pHBInfo->m_TemperatureStable_Detected = false;
        }
    }
}

float GetAcceptableTemperatureDrop() {
    if (GetBitValue(SystemCoil::print_fan_active)) {
        return TEMPERATURE_DROP_THRESHOLD_WITH_FAN_DEGREES;
    } else {
        return TEMPERATURE_DROP_THRESHOLD_WITHOUT_FAN_DEGREES;
    }
}

void CheckTemperatureDrop(uint32_t heatbedletIndex) {
    HeatbedletInfo *pHBInfo = HeatbedletInfo::Get(heatbedletIndex);

    if (pHBInfo->m_State == HeatbedletState::Stable) {
        if (pHBInfo->m_MeasuredTemperature < (pHBInfo->m_TargetTemperature - GetAcceptableTemperatureDrop())) {
            if (pHBInfo->m_TemperatureDrop_Detected == false) {
                pHBInfo->m_TemperatureDrop_Detected = true;
                pHBInfo->m_TemperatureDrop_StartTime = osKernelSysTick();
            }

            if (((int32_t)(osKernelSysTick() - pHBInfo->m_TemperatureDrop_StartTime)) >= (TEMPERATURE_DROP_THRESHOLD_SECONDS * TICKS_PER_SECOND)) {
                SetHBErrorFlag(heatbedletIndex, HeatbedletError::TemperatureDropDetected);
            }
        } else {
            pHBInfo->m_TemperatureDrop_Detected = false;
        }
    }
}

void CheckTemperaturePeak(uint32_t heatbedletIndex) {
    HeatbedletInfo *pHBInfo = HeatbedletInfo::Get(heatbedletIndex);

    if (pHBInfo->m_State == HeatbedletState::Stable) {
        if ((pHBInfo->m_MeasuredTemperature > TEMPERATURE_PEAK_AMBIENT_DEGREES)
            && (pHBInfo->m_MeasuredTemperature > (pHBInfo->m_TargetTemperature + TEMPERATURE_PEAK_THRESHOLD_DEGREES))) {
            if (pHBInfo->m_TemperaturePeak_Detected == false) {
                pHBInfo->m_TemperaturePeak_Detected = true;
                pHBInfo->m_TemperaturePeak_StartTime = osKernelSysTick();
            }

            if (((int32_t)(osKernelSysTick() - pHBInfo->m_TemperaturePeak_StartTime)) >= (TEMPERATURE_PEAK_THRESHOLD_SECONDS * TICKS_PER_SECOND)) {
                SetHBErrorFlag(heatbedletIndex, HeatbedletError::TemperaturePeakDetected);
            }
        } else {
            pHBInfo->m_TemperaturePeak_Detected = false;
        }
    }
}

float GetPreheatMinTemperatureIncrease() {
    if (GetBitValue(SystemCoil::print_fan_active)) {
        return TEMPERATURE_PREHEAT_CHECK_MIN_DEGREES_WITH_FAN;
    } else {
        return TEMPERATURE_PREHEAT_CHECK_MIN_DEGREES_WITHOUT_FAN;
    }
}

void StartPreheatWatching(uint32_t heatbedletIndex) {
    HeatbedletInfo *pHBInfo = HeatbedletInfo::Get(heatbedletIndex);

    pHBInfo->m_NextPreheatCheckTime = osKernelSysTick() + TEMPERATURE_PREHEAT_CHECK_SECONDS * TICKS_PER_SECOND;
    pHBInfo->m_NextPreheatCheckTemperature = pHBInfo->m_MeasuredTemperature + GetPreheatMinTemperatureIncrease();
}

void CheckPreheatError(uint32_t heatbedletIndex) {
    HeatbedletInfo *pHBInfo = HeatbedletInfo::Get(heatbedletIndex);

    if (pHBInfo->m_State == HeatbedletState::Heating && ENABLE_TEMPERATURE_CHECKS) {
        if (((int32_t)(osKernelSysTick() - pHBInfo->m_NextPreheatCheckTime)) > 0) {
            if (pHBInfo->m_MeasuredTemperature < pHBInfo->m_NextPreheatCheckTemperature) {
                SetHBErrorFlag(heatbedletIndex, HeatbedletError::PreheatError);
            }
            StartPreheatWatching(heatbedletIndex);
        }
    }
}

} // namespace modularbed::StateLogic
