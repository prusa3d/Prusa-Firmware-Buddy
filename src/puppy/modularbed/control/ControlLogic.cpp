#include "ControlLogic.hpp"
#include "HeatbedletInfo.hpp"
#include "PWMLogic.hpp"
#include "StateLogic.hpp"
#include "cmsis_os.h"
#include "PuppyConfig.hpp"
#include <cstdint>
#include <cmath>

namespace modularbed::ControlLogic {

void RunHBControllers();
void CalcHBControllers();
void CalcHBAntiWindup();
float CalcExpectedLinearPWM(float temperature);
float DelinearizePWM(float pwm);
void StoreValuesToModbusRegisters();
float LimitValue(float value, float min, float max);

static bool s_ControllersEnabled = true;

static uint32_t s_LastControllerItertionTime = 0;

void Init() {
    s_LastControllerItertionTime = osKernelSysTick();

    for (int hbIndex = 0; hbIndex < HEATBEDLET_COUNT; hbIndex++) {
        HeatbedletInfo *pHBInfo = HeatbedletInfo::Get(hbIndex);

        pHBInfo->m_PID_P_Coef = CONTROLLER_P_COEF;
        pHBInfo->m_PID_I_Coef = CONTROLLER_I_COEF;
        pHBInfo->m_PID_D_Coef = CONTROLLER_D_COEF;

        uint16_t pe = (uint16_t)(pHBInfo->m_PID_P_Coef * ((float)MODBUS_PID_SCALE) + 0.5f);

        ModbusRegisters::SetRegValue(ModbusRegisters::HBHoldingRegister::pid_p, hbIndex, pe);
        uint16_t i = (uint16_t)(pHBInfo->m_PID_I_Coef * ((float)MODBUS_PID_SCALE) + 0.5f);
        ModbusRegisters::SetRegValue(ModbusRegisters::HBHoldingRegister::pid_i, hbIndex, i);

        uint16_t d = (uint16_t)(pHBInfo->m_PID_D_Coef * ((float)MODBUS_PID_SCALE) + 0.5f);
        ModbusRegisters::SetRegValue(ModbusRegisters::HBHoldingRegister::pid_d, hbIndex, d);
    }
}

void EnableControllers() {
    s_ControllersEnabled = true;
}

void DisableControllers() {
    s_ControllersEnabled = false;
}

void IterateContollers() {
    uint32_t now = osKernelSysTick();
    uint32_t period = (TICKS_PER_SECOND / CONTROLLER_FREQUENCY);

    if ((now - s_LastControllerItertionTime) >= period) {
        s_LastControllerItertionTime = now;

        RunHBControllers();
    }
}

void RunHBControllers() {
    if (s_ControllersEnabled) {
        CalcHBControllers();
        PWMLogic::ApplyPWMValues();
        CalcHBAntiWindup();
        StoreValuesToModbusRegisters();
    }
}

void CalcHBControllers() {
    for (int hbIndex = 0; hbIndex < HEATBEDLET_COUNT; hbIndex++) {
        HeatbedletInfo *pHBInfo = HeatbedletInfo::Get(hbIndex);
        float action = 0;
        if (is_used_bedlet(hbIndex)) {
            if (pHBInfo->m_TargetTemperature == 0 || (TURN_OFF_HEATING_ON_ERROR && (pHBInfo->m_State == HeatbedletState::Error || StateLogic::IsAnyFaultActive()))) {
                // clear controller action
                if (pHBInfo->m_PID_IsON) {
                    pHBInfo->m_PID_IsON = false;
                    pHBInfo->m_PID_Err = 0;
                    pHBInfo->m_PID_P_Action = 0;
                    pHBInfo->m_PID_I_ActionDelta = 0;
                    pHBInfo->m_PID_I_Action = 0;
                    pHBInfo->m_PID_D_Action = 0;
                }
            } else {
                // initialize controller
                if (!pHBInfo->m_PID_IsON) {
                    pHBInfo->m_PID_IsON = true;
                    pHBInfo->m_PID_I_Action = CalcExpectedLinearPWM(pHBInfo->m_FilteredMeasuredTemperature - HeatbedletInfo::m_ChamberTemperature);
                }

                // calculate control error
                pHBInfo->m_PID_Err = pHBInfo->m_TargetTemperature - pHBInfo->m_FilteredMeasuredTemperature;

                // proportional component
                pHBInfo->m_PID_P_Action = pHBInfo->m_PID_P_Coef * pHBInfo->m_PID_Err;

                // integration component
                pHBInfo->m_PID_I_ActionDelta = (double)pHBInfo->m_PID_I_Coef * (double)pHBInfo->m_PID_Err / (double)CONTROLLER_FREQUENCY;
                pHBInfo->m_PID_I_Action += pHBInfo->m_PID_I_ActionDelta;

                // derivative component
                // use "derivative on measurement" trick to avoid "derivative kick" problem
                pHBInfo->m_PID_D_Action = -pHBInfo->m_PID_D_Coef * pHBInfo->m_FilteredMeasuredTemperatureDiff;

                action = pHBInfo->m_PID_P_Action + (float)pHBInfo->m_PID_I_Action + pHBInfo->m_PID_D_Action;

                // action = pHBInfo->m_TargetTemperature / 100.0f; //For testing purposes
                // action = CalcExpectedLinearPWM(pHBInfo->m_TargetTemperature - HeatbedletInfo::m_ChamberTemperature); //For testing purposes
            }

            if (action < 0) {
                action = 0;
                pHBInfo->m_IsPWMLowerSaturated = true;
            }

            if (action > 1) {
                action = 1;
                pHBInfo->m_IsPWMUpperSaturated = true;
            }

            action = DelinearizePWM(action);

            if (action > 1) {
                action = 1;
                pHBInfo->m_IsPWMUpperSaturated = true;
            }
        }
        pHBInfo->m_PWMValue = action;
    }
}

void CalcHBAntiWindup() {
    for (int hbIndex = 0; hbIndex < HEATBEDLET_COUNT; hbIndex++) {
        HeatbedletInfo *pHBInfo = HeatbedletInfo::Get(hbIndex);

        // is controller output saturated and anti-windup is needed?
        if ((pHBInfo->m_IsPWMLowerSaturated && (pHBInfo->m_PID_Err < 0)) || (pHBInfo->m_IsPWMUpperSaturated && (pHBInfo->m_PID_Err > 0))) {
            pHBInfo->m_IsPWMLowerSaturated = false;
            pHBInfo->m_IsPWMUpperSaturated = false;
            // undo last integration component change
            pHBInfo->m_PID_I_Action -= pHBInfo->m_PID_I_ActionDelta;
            pHBInfo->m_PID_I_ActionDelta = 0;
            // back-calculation anti-windup
            float deltaPWM = CalcExpectedLinearPWM(pHBInfo->m_FilteredMeasuredTemperatureDiff);
            double deltaI = (double)((deltaPWM) / CONTROLLER_FREQUENCY);
            pHBInfo->m_PID_I_Action = LimitValue(pHBInfo->m_PID_I_Action + deltaI, 0, 1);
        }
    }
}

float CalcExpectedLinearPWM(float temperature) {
    float pwm = CONTROLLER_EXPECTED_PWM_COEF_A1 * temperature;
    return pwm;
}

float DelinearizePWM(float pwm) {
    float outPWM = pwm * (pwm * CONTROLLER_PWM_DELINEAR_COEF_A2 + CONTROLLER_PWM_DELINEAR_COEF_A1);
    return outPWM;
}

void StoreValuesToModbusRegisters() {
    for (int hbIndex = 0; hbIndex < HEATBEDLET_COUNT; hbIndex++) {
        HeatbedletInfo *pHBInfo = HeatbedletInfo::Get(hbIndex);

        uint16_t p_action = (uint16_t)(LimitValue(pHBInfo->m_PID_P_Action, -1, 1) * ((float)MODBUS_PID_SCALE) + 0.5f);
        ModbusRegisters::SetRegValue(ModbusRegisters::HBInputRegister::pid_p_control_action, hbIndex, p_action);

        uint16_t i_action = (uint16_t)(LimitValue((float)pHBInfo->m_PID_I_Action, -1, 1) * ((float)MODBUS_PID_SCALE) + 0.5f);
        ModbusRegisters::SetRegValue(ModbusRegisters::HBInputRegister::pid_i_control_action, hbIndex, i_action);

        uint16_t d_action = (uint16_t)(LimitValue(pHBInfo->m_PID_D_Action, -1, 1) * ((float)MODBUS_PID_SCALE) + 0.5f);
        ModbusRegisters::SetRegValue(ModbusRegisters::HBInputRegister::pid_d_control_action, hbIndex, d_action);
    }
}

float LimitValue(float value, float min, float max) {
    if (value < min) {
        return min;
    }

    if (value > max) {
        return max;
    }

    return value;
}

} // namespace modularbed::ControlLogic
