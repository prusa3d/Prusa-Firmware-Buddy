#include <device/hal.h>
#include <device/cmsis.h>

#include "hal/HAL_System.hpp"
#include "hal/HAL_RS485.hpp"
#include "hal/HAL_ADC.hpp"
#include "hal/HAL_GPIO.hpp"
#include "hal/HAL_PWM.hpp"
#include "hal/HAL_Common.hpp"
#include "modbus/ModbusProtocol.hpp"
#include "modbus/ModbusTask.hpp"
#include "control/PWMLogic.hpp"
#include "control/ControlLogic.hpp"
#include "control/ControlTask.hpp"

#include "cmsis_os.h"
#include "stm32g0xx_hal.h"
#include "option/bootloader.h"
#include "startup/ApplicationStartupArguments.hpp"
#include "safe_state.h"
#include "bsod.h"
#include "ModbusRegisters.hpp"
#include "hal/HAL_MultiWatchdog.hpp"
#include <cpu_utils.hpp>

extern "C" {
void vApplicationStackOverflowHook(TaskHandle_t xTask, signed char *pcTaskName);
void fatal_error(const char *error, const char *module);
void __libc_init_array(void);
}

void vApplicationStackOverflowHook([[maybe_unused]] TaskHandle_t xTask, [[maybe_unused]] signed char *pcTaskName) {
    bsod("vApplicationStackOverflowHook");
}

void fatal_error(const char *error, [[maybe_unused]] const char *module) {
    bsod(error);
}

void Error_Handler(void) {
    bsod("Error_Handler");
}

void _bsod([[maybe_unused]] const char *fmt, [[maybe_unused]] const char *file_name, [[maybe_unused]] int line_number, ...) {
    hal::PWMDriver::TurnOffAll();

    // log_critical(Marlin, "BSOD");
    while (1) {
        trigger_crash_dump();
    }
}

static hal::MultiWatchdog idle_task_watchdog; // Add one instance of watchdog
static void idle_task_watchdog_callback() {
    idle_task_watchdog.kick(false); // Mark this watchdog instance, do not reload hardware from this instance
}

/// The entrypoint of our firmware
///
/// Do not do anything here that isn't essential to starting the RTOS
/// That is our one and only priority.
///
/// WARNING
/// The C++ runtime hasn't been initialized yet (together with C's constructors).
/// So make sure you don't do anything that is dependent on it.
int main(void) {
    // initialize vector table & external memory
    SystemInit();

    // initialize HAL
    HAL_StatusTypeDef res = HAL_Init();
    (void)res;
    assert(HAL_OK == res);
    hal::System::SystemClock_Config();

    __libc_init_array();

    hal::MultiWatchdog::init(); // Configure hardware watchdog
    osSetIdleTaskWatchdog(idle_task_watchdog_callback); // Add watchdog to idle task

    if (hal::GPIODriver::Init() == false) {
        Error_Handler();
    }

    if (hal::ADCDriver::Init() == false) {
        Error_Handler();
    }

    if (hal::RS485Driver::Init(get_assigned_modbus_address()) == false) {
        Error_Handler();
    }

    if (hal::PWMDriver::Init() == false) {
        Error_Handler();
    }

    modularbed::ModbusRegisters::Init();

    modbus::ModbusProtocol::Init(get_assigned_modbus_address());

    modularbed::MeasurementLogic::Init();

    modularbed::PWMLogic::Init();

    modularbed::ControlLogic::Init();

    if (modbus::ModbusTask::Init() == false) {
        Error_Handler();
    }

    if (modularbed::ControlTask::Init() == false) {
        Error_Handler();
    }

    osKernelStart(); // function never returns
}
