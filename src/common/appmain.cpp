//appmain.cpp - arduino-like app start

#include "appmain.hpp"
#include "app.h"
#include "app_metrics.h"
#include "log.h"
#include "cmsis_os.h"
#include "config.h"
#include "adc.hpp"
#include "Jogwheel.hpp"
#include "hwio.h"
#include "sys.h"
#include "gpio.h"
#include "metric.h"
#include "print_utils.hpp"
#include "sound.hpp"
#include "language_eeprom.hpp"

#include "marlin_server.h"
#include "bsod.h"
#include "eeprom.h"
#include "safe_state.h"
#include "crc32.h"
#include <crash_dump/dump.h>

#include <Arduino.h>
#include "trinamic.h"
#include "../Marlin/src/module/settings.h"
#include <tasks.h>

#if ENABLED(POWER_PANIC)
    #include "power_panic.hpp"
#endif

#ifdef NEW_FANCTL
    #include "fanctl.h"
    #include "hwio_pindef.h"

//condition to restore autofan after selftest
static bool restore_autofan() { return marlin_server_get_temp_nozzle() >= EXTRUDER_AUTO_FAN_TEMPERATURE; }

CFanCtl fanCtlPrint = CFanCtl(
    buddy::hw::fanPrintPwm,
    #if (BOARD_IS_BUDDY)
    buddy::hw::fanPrintTach,
    #else
    buddy::hw::fanTach,
    #endif
    FANCTLPRINT_PWM_MIN,
    FANCTLPRINT_PWM_MAX,
    FANCTLPRINT_RPM_MIN, FANCTLPRINT_RPM_MAX,
    FANCTLPRINT_PWM_THR,
    is_autofan_t::no, restore_autofan,
    #if (BOARD_IS_BUDDY)
    skip_tacho_t::no
    #else
    skip_tacho_t::yes
    #endif
);
CFanCtl fanCtlHeatBreak = CFanCtl(
    buddy::hw::fanHeatBreakPwm,
    #if (BOARD_IS_BUDDY)
    buddy::hw::fanHeatBreakTach,
    #else
    buddy::hw::fanTach,
    #endif
    FANCTLHEATBREAK_PWM_MIN, FANCTLHEATBREAK_PWM_MAX,
    FANCTLHEATBREAK_RPM_MIN, FANCTLHEATBREAK_RPM_MAX,
    FANCTLHEATBREAK_PWM_THR,
    is_autofan_t::yes, restore_autofan,
    skip_tacho_t::no);
#endif //NEW_FANCTL

LOG_COMPONENT_DEF(Marlin, LOG_SEVERITY_INFO);
LOG_COMPONENT_DEF(Buddy, LOG_SEVERITY_DEBUG);
LOG_COMPONENT_DEF(Core, LOG_SEVERITY_INFO);

extern void reset_trinamic_drivers();

extern "C" {

#ifndef USE_ESP01_WITH_UART6
extern uartslave_t uart6slave; // PUT slave
#endif

#ifdef BUDDY_ENABLE_ETHERNET
extern osThreadId webServerTaskHandle; // Webserver thread(used for fast boot mode)
#endif                                 //BUDDY_ENABLE_ETHERNET

void app_marlin_serial_output_write_hook(const uint8_t *buffer, int size) {
    while (size && (buffer[size - 1] == '\n' || buffer[size - 1] == '\r'))
        size--;
    log_severity_t severity = LOG_SEVERITY_INFO;
    if (size == 2 && memcmp("ok", buffer, 2) == 0) {
        // Do not log "ok" messages
        return;
    } else if (size >= 5 && memcmp("echo:", buffer, 5) == 0) {
        buffer = buffer + 5;
        size -= 5;
    }
    if (size >= 6 && memcmp("Error:", buffer, 6) == 0) {
        buffer = buffer + 6;
        size -= 6;
        severity = LOG_SEVERITY_ERROR;
    }
    log_event(severity, Marlin, "%.*s", size, buffer);
}

void app_setup_marlin_logging() {
    SerialUSB.lineBufferHook = app_marlin_serial_output_write_hook;
}

void app_startup() {
    app_setup_marlin_logging();
    log_info(Buddy, "marlin task waiting for dependecies");
    wait_for_dependecies(DEFAULT_TASK_DEPS);
    log_info(Buddy, "marlin task is starting");
}

void app_setup(void) {
    if (INIT_TRINAMIC_FROM_MARLIN_ONLY == 0) {
        init_tmc();
    } else {
        init_tmc_bare_minimum();
    }

    setup();

    marlin_server_settings_load(); // load marlin variables from eeprom
}

void app_idle(void) {
    Buddy::Metrics::RecordMarlinVariables();
    Buddy::Metrics::RecordRuntimeStats();
    Buddy::Metrics::RecordPrintFilename();
    print_utils_loop();
    osDelay(0); // switch to other threads - without this is UI slow during printing
}

void app_run(void) {
    LangEEPROM::getInstance();

    marlin_server_init();
    marlin_server_idle_cb = app_idle;

    log_info(Marlin, "Starting setup");

    app_setup();

    marlin_server_start_processing();

    log_info(Marlin, "Setup complete");

    if (eeprom_init() == EEPROM_INIT_Defaults && marlin_server_processing()) {
        settings.reset();
#if ENABLED(POWER_PANIC)
        power_panic::reset();
#endif
    }

    while (1) {
        if (marlin_server_processing()) {
            loop();
        }
#ifndef USE_ESP01_WITH_UART6
        uartslave_cycle(&uart6slave);
#endif
        marlin_server_loop();
    }
}

void app_error(void) {
    bsod("app_error");
}

void app_assert(uint8_t *file, uint32_t line) {
    bsod("app_assert");
}

#ifdef NEW_FANCTL
static void record_fanctl_metrics() {
    static metric_t metric = METRIC("fan", METRIC_VALUE_CUSTOM, 0, METRIC_HANDLER_ENABLE_ALL);
    static uint32_t last_update = 0;

    auto record = [](CFanCtl &fanctl, const char *fan_name) {
        auto fanStateToInt = [](CFanCtl::FanState state) {
            switch (state) {
            case CFanCtl::FanState::idle:
                return 0;
            case CFanCtl::FanState::starting:
                return 1;
            case CFanCtl::FanState::running:
                return 2;
            default:
                return -1;
            }
        };
        int state = fanStateToInt(fanctl.getState());
        float pwm = static_cast<float>(fanctl.getPWM()) / static_cast<float>(fanctl.getMaxPWM());
        float measured = static_cast<float>(fanctl.getActualRPM()) / static_cast<float>(fanctl.getMaxRPM());

        metric_record_custom(&metric, ",fan=%s state=%i,pwm=%i,measured=%i",
            fan_name, state, (int)(pwm * 100.0f), (int)(measured * 100.0f));
    };

    if (HAL_GetTick() - last_update > 987) {
        record(fanCtlPrint, "print");
        record(fanCtlHeatBreak, "heatbreak");
        last_update = HAL_GetTick();
    }
}
#endif

void adc_tick_1ms(void) {
}

void app_tim14_tick(void) {
#ifdef NEW_FANCTL
    fanctl_tick();
    record_fanctl_metrics();
#endif //NEW_FANCTL
#if HAS_GUI()
    jogwheel.Update1msFromISR();
#endif
    Sound_Update1ms();
    //hwio_update_1ms();
    adc_tick_1ms();
}

} // extern "C"

//cpp code
