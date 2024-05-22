#include <device/hal.h>
#include <device/cmsis.h>
#include "cmsis_os.h"
#include "SEGGER_SYSVIEW.h"
#include "SEGGER_RTT.h"
#include <assert.h>
#include "task_startup.h"
#include "led.h"
#include "timing_sys.h"
#include "trigger_crash_dump.h"
#include "safe_state.h"
#include "bsod.h"
#include "log.h"
#include "buddy/priorities_config.h"

LOG_COMPONENT_REF(Marlin);

static void enable_backup_domain() {
    // this allows us to use the RTC->BKPXX registers
    __HAL_RCC_PWR_CLK_ENABLE();
    HAL_PWR_EnableBkUpAccess();
}

static void enable_segger_sysview() {
    SEGGER_SYSVIEW_Conf();
}

/// The (C) entrypoint of the startup task
///
/// WARNING
/// The C++ runtime isn't initialized at the beginning of this function
/// and initializing it is the main priority here.
extern "C" void startup_task_entry([[maybe_unused]] void const *argument) {
    // init global variables and call constructors
    extern void __libc_init_array(void);
    __libc_init_array();

    // Jump to the main C++ implementation
    startup_task_run();

    // terminate this thread (release its resources), we are done
    osThreadTerminate(osThreadGetId());
}

void system_core_error_handler() {
    bsod("system_core_error_handler");
}

extern "C" void vApplicationStackOverflowHook([[maybe_unused]] TaskHandle_t xTask, [[maybe_unused]] signed char *pcTaskName) {
    bsod("vApplicationStackOverflowHook");
}

extern "C" __attribute__((noreturn)) void fatal_error(const char *error, [[maybe_unused]] const char *module) {
    bsod(error);
}

extern "C" void Error_Handler(void) {
    bsod("Error_Handler");
}

// general
void _bsod([[maybe_unused]] const char *fmt, [[maybe_unused]] const char *file_name, [[maybe_unused]] int line_number, ...) {
    hwio_safe_state();
    log_critical(Marlin, "BSOD");
    while (1) {
        led::set_rgb(0x08, 0, 0x04);
        trigger_crash_dump();
    }
}

/// The entrypoint of our firmware
///
/// Do not do anything here that isn't essential to starting the RTOS
/// That is our one and only priority.
///
/// WARNING
/// The C++ runtime hasn't been initialized yet (together with C's constructors).
/// So make sure you don't do anything that is dependent on it.
int main() {
    // initialize vector table & external memory
    SystemInit();

    // initialize HAL
    HAL_StatusTypeDef res = HAL_Init();
    (void)res;
    assert(HAL_OK == res);

    // configure system clock and timing
    system_core_init();

    // other MCU setup
    enable_backup_domain();
    enable_segger_sysview();

    // define the startup task
    osThreadDef(startup, startup_task_entry, TASK_PRIORITY_STARTUP, 0, 512);
    osThreadCreate(osThread(startup), NULL);

    // start the RTOS with the single startup task
    osKernelStart();
}
