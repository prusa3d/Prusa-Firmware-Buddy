#include "ModbusTask.hpp"
#include "cmsis_os.h"
#include "device/hal.h"
#include <cstring>
#include <hal/HAL_MultiWatchdog.hpp>
#include "buddy/priorities_config.h"

namespace modbus::ModbusTask {

#define MODBUS_WATCHDOG_CHECK_PERIOD 500

extern "C" {
void ModbusTaskFunction(const void *argument);
}

static void OnReceive(uint8_t *pData, uint32_t dataSize);

static void ResetModbusWatchdog();
static void CheckModbusWatchdog();

static bool s_ModbusWatchdogActive = false;
static uint32_t s_ModbusWatchdogStartTime = 0;

static osSemaphoreId s_RXDataSemaphore = nullptr;
static osSemaphoreDef(s_RXDataSemaphore);

static osThreadId s_OSThredHandle = nullptr;
osThreadDef(ModbusTask, ModbusTaskFunction, TASK_PRIORITY_MODBUS, 1, (256 * 4));

static bool s_ModbusEnabled = false;
static bool s_ExitThread = false;

static ModbusBuffer s_RX_Buffer;
static ModbusBuffer m_TX_Buffer;

bool Init() {
    s_RXDataSemaphore = osSemaphoreCreate(osSemaphore(s_RXDataSemaphore), 1);
    if (s_RXDataSemaphore == nullptr) {
        return false;
    }

    s_OSThredHandle = osThreadCreate(osThread(ModbusTask), NULL);
    if (s_OSThredHandle == nullptr) {
        return false;
    }

    return true;
}

void OnReceive(uint8_t *pData, uint32_t dataSize) {
    // WARNING: this method is called form USART IRQ handler

    if (dataSize > RS485_BUFFER_SIZE) {
        dataSize = RS485_BUFFER_SIZE;
    }

    if (s_RX_Buffer.GetActualSize() == 0) {
        s_RX_Buffer.CopyData(pData, dataSize);
        osSemaphoreRelease(s_RXDataSemaphore);
    }
}

void EnableModbus() {
    s_ModbusEnabled = true;
}

void DisableModbus() {
    s_ModbusEnabled = false;
}

void ModbusTaskFunction([[maybe_unused]] const void *argument) {
    hal::MultiWatchdog wdg; // Add one instance of watchdog
    hal::RS485Driver::SetOnReceiveCallback(OnReceive);
    hal::RS485Driver::StartReceiving();

    while (s_ExitThread == false) {
        if (osSemaphoreWait(s_RXDataSemaphore, MODBUS_WATCHDOG_CHECK_PERIOD) == osOK) {

            if (s_ModbusEnabled) {
                if (modbus::ModbusProtocol::ProcessFrame(&s_RX_Buffer, &m_TX_Buffer)) {
                    hal::RS485Driver::Transmit(&m_TX_Buffer[0], m_TX_Buffer.GetActualSize());

                    ResetModbusWatchdog();
                }
            }

            s_RX_Buffer.Reset();
        }

        wdg.kick(); // Reload this instance of watchdog
        CheckModbusWatchdog();
    }
}

void ResetModbusWatchdog() {
    s_ModbusWatchdogActive = true;
    s_ModbusWatchdogStartTime = osKernelSysTick();
}

void CheckModbusWatchdog() {
    uint32_t now = osKernelSysTick();

    if (s_ModbusWatchdogActive) {
        if (((int32_t)(now - s_ModbusWatchdogStartTime)) > MODBUS_WATCHDOG_TIMEOUT) {
            HAL_NVIC_SystemReset();
        }
    }
}

} // namespace modbus::ModbusTask
