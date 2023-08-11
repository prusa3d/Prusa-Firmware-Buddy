#include "ControlTask.hpp"
#include "MeasurementCalculator.hpp"
#include "hal/HAL_System.hpp"
#include "hal/HAL_GPIO.hpp"
#include "hal/HAL_OTP.hpp"
#include "modbus/ModbusTask.hpp"
#include "ControlLogic.hpp"
#include "PWMLogic.hpp"
#include "HeatbedletInfo.hpp"
#include <cstring>
#include "hal/HAL_MultiWatchdog.hpp"
#include "buddy/priorities_config.h"

#define MODBUS_QUEUE_MESSAGE_COUNT 40
#define ALL_ERROR_BITS             0xFFFFFFFF

namespace modularbed::ControlTask {

struct ModbusMessage {
    uint16_t m_Address;
    uint32_t m_Value;
};

extern "C" {
void ControlTaskFunction(const void *argument);
}

static osThreadId s_OSThredHandle = nullptr;
osThreadDef(ControlTask, ControlTaskFunction, TASK_PRIORITY_CONTROL, 1, (256 * 4));
static bool s_ExitThread = false;

static osMailQId s_ModbusQueueHandle = nullptr;
osMailQDef(s_ModbusQueueHandle, MODBUS_QUEUE_MESSAGE_COUNT, ModbusMessage);

void ProcessADCMeasurements();
void ProcessSignals();
void ProcessModbusMessages();
bool IsHBRegister(uint16_t address, uint16_t firstRegister, uint16_t *pHBIndex);

void OnWriteCoil(uint16_t address, bool value);
void OnWriteRegister(uint16_t address, uint16_t value);
bool OnReadFileRecord(uint16_t fileNumber, uint16_t recordNumber, uint16_t recordLength, modbus::ModbusBuffer *pBuffer);
bool OnWriteFileRecord(uint16_t fileNumber, uint16_t recordNumber, uint16_t recordLength, modbus::ModbusBuffer *pBuffer);

bool Init() {
    s_OSThredHandle = osThreadCreate(osThread(ControlTask), NULL);
    if (s_OSThredHandle == nullptr) {
        return false;
    }

    s_ModbusQueueHandle = osMailCreate(osMailQ(s_ModbusQueueHandle), nullptr);
    if (s_ModbusQueueHandle == nullptr) {
        return false;
    }

    modbus::ModbusProtocol::SetOnWriteCoilCallback(OnWriteCoil);
    modbus::ModbusProtocol::SetOnWriteRegisterCallback(OnWriteRegister);
    modbus::ModbusProtocol::SetOnReadFileRecordCallback(OnReadFileRecord);
    modbus::ModbusProtocol::SetOnWriteFileRecordCallback(OnWriteFileRecord);

    return true;
}

void OnWriteCoil(uint16_t address, bool value) {
    // WARNING: this method is called from different thread

    ModbusMessage *pMsg = (ModbusMessage *)osMailAlloc(s_ModbusQueueHandle, osWaitForever);
    pMsg->m_Address = address;
    pMsg->m_Value = value;
    osMailPut(s_ModbusQueueHandle, pMsg);
}

void OnWriteRegister(uint16_t address, uint16_t value) {
    // WARNING: this method is called from different thread
    ModbusMessage *pMsg = (ModbusMessage *)osMailAlloc(s_ModbusQueueHandle, osWaitForever);
    pMsg->m_Address = address;
    pMsg->m_Value = value;
    osMailPut(s_ModbusQueueHandle, pMsg);
}

bool OnReadFileRecord(uint16_t fileNumber, uint16_t recordNumber, uint16_t recordLength, modbus::ModbusBuffer *pBuffer) {
    // WARNING: this method is called from different thread

    if (fileNumber != MODBUS_OTP_FILE_NUMBER) {
        return false;
    }

    uint32_t buf = 0;
    for (int i = 0; i < recordLength; i++) {
        if (hal::OTPDriver::ReadOTPMemory(((recordNumber * recordLength) + i) * 2, &buf, 2) == false) {
            return false;
        }
        pBuffer->AddWord((uint16_t)buf);
    }

    return true;
}

bool OnWriteFileRecord([[maybe_unused]] uint16_t fileNumber, [[maybe_unused]] uint16_t recordNumber, [[maybe_unused]] uint16_t recordLength, [[maybe_unused]] modbus::ModbusBuffer *pBuffer) {
    // WARNING: this method is called from different thread

    return false;
}

void ControlTaskFunction([[maybe_unused]] const void *argument) {
    ProcessSignals();
    MeasurementLogic::CalibrateCurrentChannels();
    AutoconfLogic::CheckHeatbedlets();
    AutoconfLogic::MeasureAndCheckAllHBCurrents();
    MeasurementLogic::StartADCMeasurements();
    modbus::ModbusTask::EnableModbus();
    hal::MultiWatchdog wdg; // Add one instance of watchdog

    while (s_ExitThread == false) {
        ProcessADCMeasurements();
        ProcessSignals();
        ProcessModbusMessages();
        AutoconfLogic::IterateTesting();
        ControlLogic::IterateContollers();

        wdg.kick(); // Reload this instance of watchdog
        osDelay(1);
    }
}

void ProcessADCMeasurements() {
    hal::ADCDriver::ADCChannel channel;
    float value;

    if (MeasurementLogic::IterateADCMeasurements(&channel, &value)) {
        StateLogic::ProcessMeasuredValue(channel, value);
    }
}

void ProcessSignals() {
    bool panic = hal::GPIODriver::ReadPANICSignal();
    bool fault = hal::GPIODriver::ReadFAULTSignal();

    StateLogic::SignalsRefreshed(panic, fault);
}

bool IsHBRegister(uint16_t address, uint16_t firstRegister, uint16_t *pHBIndex) {
    uint16_t hbIndex = address - firstRegister;

    if (hbIndex < HEATBEDLET_COUNT) {
        *pHBIndex = hbIndex;
        return true;
    } else {
        return false;
    }
}

void ProcessModbusMessages() {
    osEvent event;
    ModbusMessage *pMsg;
    uint16_t hbIndex;

    while ((event = osMailGet(s_ModbusQueueHandle, 0)).status == osEventMail) {
        pMsg = (ModbusMessage *)event.value.p;
        switch (pMsg->m_Address) {
        case ((uint16_t)ModbusRegisters::SystemCoil::clear_fault_status):
            StateLogic::ClearSystemErrorBits(ALL_ERROR_BITS);
            break;
        case ((uint16_t)ModbusRegisters::SystemCoil::reset_overcurrent_fault):
            hal::GPIODriver::ResetOverCurrentFault();
            ProcessSignals();
            break;
        case ((uint16_t)ModbusRegisters::SystemCoil::test_hb_heating):
            if (pMsg->m_Value != 0) {
                AutoconfLogic::StartTestHBHeating();
            } else {
                AutoconfLogic::StopTestHBHeating();
            }
            break;
        case ((uint16_t)ModbusRegisters::SystemCoil::measure_hb_currents):
            if (pMsg->m_Value != 0) {
                AutoconfLogic::MeasureAndCheckAllHBCurrents();
                ModbusRegisters::SetBitValue(ModbusRegisters::SystemCoil::measure_hb_currents, false);
            }
            break;
        case ((uint16_t)ModbusRegisters::SystemCoil::calibrate_thermistors):
            if (pMsg->m_Value != 0) {
                AutoconfLogic::CalibrateThermistors();
                ModbusRegisters::SetBitValue(ModbusRegisters::SystemCoil::calibrate_thermistors, false);
            }
            break;
        case ((uint16_t)ModbusRegisters::SystemHoldingRegister::chamber_temperature):
            HeatbedletInfo::m_ChamberTemperature = ((float)pMsg->m_Value) / MODBUS_TEMPERATURE_REGISTERS_SCALE;
            break;
        case ((uint16_t)ModbusRegisters::SystemHoldingRegister::clear_system_fault_bits):
            StateLogic::ClearSystemErrorBits(pMsg->m_Value);
            break;
        default:
            if (IsHBRegister(pMsg->m_Address, (uint16_t)ModbusRegisters::HBHoldingRegister::target_temperature, &hbIndex)) {
                HeatbedletInfo *pHBInfo = HeatbedletInfo::Get(hbIndex);
                float temp = ((float)pMsg->m_Value) / MODBUS_TEMPERATURE_REGISTERS_SCALE;

                if (temp > 0 && pHBInfo->m_MeasureResistance && pHBInfo->IsTempSensorOK()) {
                    AutoconfLogic::MeasureAndCheckSingleHBCurrent(hbIndex);
                }

                StateLogic::TargetTemperatureChanged(hbIndex, temp);
            } else if (IsHBRegister(pMsg->m_Address, (uint16_t)ModbusRegisters::HBCoil::clear_fault_status, &hbIndex)) {
                StateLogic::ClearHBErrorBits(hbIndex, ALL_ERROR_BITS);
            } else if (IsHBRegister(pMsg->m_Address, (uint16_t)ModbusRegisters::HBHoldingRegister::max_allowed_current, &hbIndex)) {
                HeatbedletInfo::Get(hbIndex)->m_MaxAllowedCurrent = (((float)pMsg->m_Value) / MODBUS_CURRENT_REGISTERS_SCALE);
                PWMLogic::ApplyPWMValues();
            } else if (IsHBRegister(pMsg->m_Address, (uint16_t)ModbusRegisters::HBHoldingRegister::pid_p, &hbIndex)) {
                HeatbedletInfo::Get(hbIndex)->m_PID_P_Coef = (((float)pMsg->m_Value) / MODBUS_PID_SCALE);
            } else if (IsHBRegister(pMsg->m_Address, (uint16_t)ModbusRegisters::HBHoldingRegister::pid_i, &hbIndex)) {
                HeatbedletInfo::Get(hbIndex)->m_PID_I_Coef = (((float)pMsg->m_Value) / MODBUS_PID_SCALE);
            } else if (IsHBRegister(pMsg->m_Address, (uint16_t)ModbusRegisters::HBHoldingRegister::pid_d, &hbIndex)) {
                HeatbedletInfo::Get(hbIndex)->m_PID_D_Coef = (((float)((int16_t)pMsg->m_Value)) / MODBUS_PID_SCALE);
            } else if (IsHBRegister(pMsg->m_Address, (uint16_t)ModbusRegisters::HBHoldingRegister::clear_hb_fault_bits, &hbIndex)) {
                StateLogic::ClearHBErrorBits(hbIndex, pMsg->m_Value);
            } else if (IsHBRegister(pMsg->m_Address, (uint16_t)ModbusRegisters::HBHoldingRegister::measured_max_current, &hbIndex)) {
                float maxCurrent = ((float)pMsg->m_Value) / MODBUS_CURRENT_REGISTERS_SCALE;
                float resistance = ((float)HEATBEDLET_VOLTAGE) / maxCurrent;
                HeatbedletInfo::Get(hbIndex)->m_ReferenceResistance = resistance;
                HeatbedletInfo::Get(hbIndex)->m_ReferenceMaxCurrent = maxCurrent;
            }
            break;
        }
        osMailFree(s_ModbusQueueHandle, pMsg);
    }
}

} // namespace modularbed::ControlTask
