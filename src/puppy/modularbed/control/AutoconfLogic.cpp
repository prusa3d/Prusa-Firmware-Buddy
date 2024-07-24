#include "cmsis_os.h"
#include <cmath>
#include "AutoconfLogic.hpp"
#include "MeasurementCalculator.hpp"
#include "PWMLogic.hpp"
#include "ControlLogic.hpp"
#include "hal/HAL_System.hpp"
#include "ModbusRegisters.hpp"

#define MAX_TEMPERATURE_FOR_THERMISTOR_CALIBRATION 35

namespace modularbed::AutoconfLogic {

static bool s_HBHeatingTestIsRunning = false;
static uint32_t s_HBHeatingTestStartTime = 0;
static float s_HBTemperatures[HEATBEDLET_COUNT];

static void MeasureAndCheckHBResistance_Impl(uint32_t hbIndex);

void CheckHeatbedlets() {
    for (int hbIndex = 0; hbIndex < HEATBEDLET_COUNT; hbIndex++) {
        ModbusRegisters::SetBitValue(ModbusRegisters::HBDiscreteInput::is_ready, hbIndex, true);

        hal::ADCDriver::ADCChannel channel = (hal::ADCDriver::ADCChannel)(((int)hal::ADCDriver::ADCChannel::Heatbedlet_0) + hbIndex);
        float temperature = MeasurementLogic::MeasureSingleChannel(channel);
        StateLogic::ProcessMeasuredValue(channel, temperature);
    }
}

void MeasureAndCheckAllHBCurrents() {
    // method takes approximately 1700 milliseconds on STM32G0@56MHz
    ControlLogic::DisableControllers();

    // turn off PWM for all heatbedlets
    for (int hbIndex = 0; hbIndex < HEATBEDLET_COUNT; hbIndex++) {
        HeatbedletInfo::Get(hbIndex)->m_PWMValue = 0;
    }

    // iterate all heatbedlets...
    for (int hbIndex = 0; hbIndex < HEATBEDLET_COUNT; hbIndex++) {
        MeasureAndCheckHBResistance_Impl(hbIndex);
    }

    PWMLogic::ApplyPWMValuesWithoutLimiters();
    ControlLogic::EnableControllers();
}

void MeasureAndCheckSingleHBCurrent(uint32_t hbIndex) {
    ControlLogic::DisableControllers();

    // turn off PWM for all heatbedlets
    for (int i = 0; i < HEATBEDLET_COUNT; i++) {
        HeatbedletInfo::Get(i)->m_PWMValue = 0;
    }

    MeasureAndCheckHBResistance_Impl(hbIndex);

    PWMLogic::ApplyPWMValuesWithoutLimiters();
    ControlLogic::EnableControllers();
}

void MeasureAndCheckHBResistance_Impl(uint32_t hbIndex) {
    HeatbedletInfo *pHBInfo = HeatbedletInfo::Get(hbIndex);

    float resistance = HEATBEDLET_DEFAULT_RESISTANCE;

    bool connected = false;
    bool disconnected = false;

    if (pHBInfo->IsTempSensorOK() && !StateLogic::IsPowerPanicActive()) {
        // set maximum PWM for iterated bed
        HeatbedletInfo::Get(hbIndex)->m_PWMValue = 1;
        PWMLogic::ApplyPWMValuesWithoutLimiters();

        hal::ADCDriver::ADCChannel currChannel = PWMLogic::GetHBCurrentMeasurementChannel(hbIndex);
        hal::ADCDriver::ADCChannel tempChannel = (hal::ADCDriver::ADCChannel)(((int)hal::ADCDriver::ADCChannel::Heatbedlet_0) + hbIndex);

        float current = MeasurementLogic::PreciselyMeasureChannel(currChannel);
        float temperature = MeasurementLogic::MeasureSingleChannel(tempChannel);
        // turn off PWM for iterated bed
        HeatbedletInfo::Get(hbIndex)->m_PWMValue = 0;
        HeatbedletInfo::Get(hbIndex)->m_MeasureResistance = false;

        if (current < 0) {
            current = 0;
        }

        bool heater_connected = false;
        bool ntc_connected = false;

        if (current >= CURRENT_MIN) {
            heater_connected = true;
        }

        if (temperature >= TEMPERATURE_MIN) {
            ntc_connected = true;
        }

        connected = heater_connected && ntc_connected;
        disconnected = !(heater_connected || ntc_connected);

        if (connected) {
            resistance = CalcHBReferenceResistance(current, temperature);
        } else {
            resistance = INF_RESISTANCE;
        }

        if (resistance > INF_RESISTANCE) {
            resistance = INF_RESISTANCE;
        }
    }

    // store measured resistance and max current
    pHBInfo->m_ReferenceResistance = resistance;
    pHBInfo->m_ReferenceMaxCurrent = ((float)HEATBEDLET_VOLTAGE) / resistance;
    ModbusRegisters::SetRegValue(ModbusRegisters::HBHoldingRegister::measured_max_current, hbIndex, (uint16_t)(pHBInfo->m_ReferenceMaxCurrent * MODBUS_CURRENT_REGISTERS_SCALE + 0.5f));

    // check HB resistance
    if (resistance < MIN_HB_RESISTANCE) {
        StateLogic::SetHBErrorFlag(hbIndex, HeatbedletError::HeaterShortCircuit);
    }

    if (is_used_bedlet(hbIndex)) {
        if (!connected || (resistance > MAX_HB_RESISTANCE)) {
            StateLogic::SetHBErrorFlag(hbIndex, HeatbedletError::HeaterDisconnected);
        }
    } else {
        if (!disconnected) {
            StateLogic::SetHBErrorFlag(hbIndex, HeatbedletError::HeaterConnected);
        }
    }
}

void CalibrateThermistors() {
    float sum = 0;
    // measure actual resistance of all thermistors
    for (int hbIndex = 0; hbIndex < HEATBEDLET_COUNT; hbIndex++) {
        HeatbedletInfo *pHBInfo = HeatbedletInfo::Get(hbIndex);
        float resistance = pHBInfo->m_FilteredThermistorResistance.GetValue();
        float temperature = CalcThermistorTemperature(resistance);
        if (temperature > MAX_TEMPERATURE_FOR_THERMISTOR_CALIBRATION) {
            return;
        }
        sum += resistance;
    }

    float average = sum / HEATBEDLET_COUNT;

    // calculate and set calibration coefficient
    for (int hbIndex = 0; hbIndex < HEATBEDLET_COUNT; hbIndex++) {
        HeatbedletInfo *pHBInfo = HeatbedletInfo::Get(hbIndex);
        pHBInfo->m_ThermistorCalibrationCoef = average / pHBInfo->m_FilteredThermistorResistance.GetValue();
    }
}

void StartTestHBHeating() {
    if (s_HBHeatingTestIsRunning) {
        return;
    }

    s_HBHeatingTestIsRunning = true;
    s_HBHeatingTestStartTime = osKernelSysTick();

    ControlLogic::DisableControllers();

    // calculate desired PWM for test
    float pwm = ((float)AUTOCONFIG_HB_HEATING_PWM_PERCENT) / 100;
    if (pwm > 1) {
        pwm = 1;
    }

    // turn off PWM for all heatbedlets
    for (int hbIndex = 0; hbIndex < HEATBEDLET_COUNT; hbIndex++) {
        HeatbedletInfo::Get(hbIndex)->m_PWMValue = 0;
    }
    PWMLogic::ApplyPWMValuesWithoutLimiters();

    // measure current temperature of all ready heatbedlets
    for (int hbIndex = 0; hbIndex < HEATBEDLET_COUNT; hbIndex++) {
        HeatbedletInfo *pHBInfo = HeatbedletInfo::Get(hbIndex);
        if (pHBInfo->IsTempSensorOK() && pHBInfo->IsHeaterOK()) {
            hal::ADCDriver::ADCChannel tempChannel = (hal::ADCDriver::ADCChannel)(((int)hal::ADCDriver::ADCChannel::Heatbedlet_0) + hbIndex);
            float temperature = MeasurementLogic::MeasureSingleChannel(tempChannel);
            s_HBTemperatures[hbIndex] = temperature;
        }
    }

    // set PWM for all heatbedlets
    for (int hbIndex = 0; hbIndex < HEATBEDLET_COUNT; hbIndex++) {
        HeatbedletInfo *pHBInfo = HeatbedletInfo::Get(hbIndex);
        if (pHBInfo->IsTempSensorOK() && pHBInfo->IsHeaterOK()) {
            HeatbedletInfo::Get(hbIndex)->m_PWMValue = pwm;
        } else {
            HeatbedletInfo::Get(hbIndex)->m_PWMValue = 0;
        }
    }

    PWMLogic::ApplyPWMValuesWithoutLimiters();
}

void StopTestHBHeating() {
    if (s_HBHeatingTestIsRunning) {
        s_HBHeatingTestIsRunning = false;
        ModbusRegisters::SetBitValue(ModbusRegisters::SystemCoil::test_hb_heating, false);

        // turn off PWM for all heatbedlets
        for (int hbIndex = 0; hbIndex < HEATBEDLET_COUNT; hbIndex++) {
            HeatbedletInfo::Get(hbIndex)->m_PWMValue = 0;
        }

        PWMLogic::ApplyPWMValues();
        ControlLogic::EnableControllers();
    }
}

void IterateTesting() {
    if (s_HBHeatingTestIsRunning && (osKernelSysTick() - s_HBHeatingTestStartTime) > (AUTOCONFIG_HB_HEATING_SECONDS * TICKS_PER_SECOND)) {
        s_HBHeatingTestIsRunning = false;
        ModbusRegisters::SetBitValue(ModbusRegisters::SystemCoil::test_hb_heating, false);

        // measure heatbedlet temperature again
        // and check whether tmeperature is rising
        for (int hbIndex = 0; hbIndex < HEATBEDLET_COUNT; hbIndex++) {
            HeatbedletInfo *pHBInfo = HeatbedletInfo::Get(hbIndex);
            if (pHBInfo->IsTempSensorOK() && pHBInfo->IsHeaterOK()) {
                hal::ADCDriver::ADCChannel tempChannel = (hal::ADCDriver::ADCChannel)(((int)hal::ADCDriver::ADCChannel::Heatbedlet_0) + hbIndex);
                float temperature = MeasurementLogic::MeasureSingleChannel(tempChannel);

                if ((temperature - s_HBTemperatures[hbIndex]) < AUTOCONFIG_HB_HEATING_MIN_TEMPERATURE_RAISE) {
                    StateLogic::SetHBErrorFlag(hbIndex, HeatbedletError::TestHeatingError);
                }
            }
        }

        // turn off PWM for all heatbedlets
        for (int hbIndex = 0; hbIndex < HEATBEDLET_COUNT; hbIndex++) {
            HeatbedletInfo::Get(hbIndex)->m_PWMValue = 0;
        }

        PWMLogic::ApplyPWMValues();
        ControlLogic::EnableControllers();
    }
}

} // namespace modularbed::AutoconfLogic
