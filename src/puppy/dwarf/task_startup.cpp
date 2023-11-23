#include "task_startup.h"
#include "logging/log.h"
#include "logging.hpp"
#include "ModbusInit.hpp"
#include "cmsis_os.h"
#include "module_marlin.hpp"
#include <device/peripherals.h>
#include "loadcell.hpp"
#include "accelerometer.hpp"
#include "adc.hpp"
#include "Cheese.hpp"
#include <hal/HAL_MultiWatchdog.hpp>
#include <cpu_utils.hpp>

LOG_COMPONENT_DEF(Dwarf, LOG_SEVERITY_INFO);

volatile bool dwarf_init_done = false;

static hal::MultiWatchdog idle_task_watchdog; // Add one instance of watchdog
static void idle_task_watchdog_callback() {
    idle_task_watchdog.kick(false); // Mark this watchdog instance, do not reload hardware from this instance
}

void startup_task_run() {
    hw_gpio_init();
    hw_dma_init();
    hw_adc1_init();
    adcDma1.init();
    SPI_INIT(accelerometer);

    hal::MultiWatchdog::init(); // Configure hardware watchdog
    osSetIdleTaskWatchdog(idle_task_watchdog_callback); // Add watchdog to idle task

    dwarf::logging_init();
    Cheese::update();

    dwarf::modbus_init();

    dwarf::loadcell::loadcell_init();

    // Check the cause of reset
    uint32_t reset_source = HAL_RCC_GetResetSource();
    if (reset_source & RCC_RESET_FLAG_IWDG) {
        log_error(Dwarf, "Dwarf was reset by its watchdog.");
    }
    // TODO: Check other reset flags here.

    dwarf_init_done = true;

    osThreadSetPriority(osThreadGetId(), osPriorityAboveNormal);
    dwarf::modules::marlin::start();
}
