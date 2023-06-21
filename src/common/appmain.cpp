// appmain.cpp - arduino-like app start

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
#include "cpu_utils.hpp"
#include "print_utils.hpp"
#include "sound.hpp"
#include "language_eeprom.hpp"
#include <device/board.h>

#if HAS_ADVANCED_POWER
    #include "advanced_power.hpp"
#endif // HAS_ADVANCED_POWER

#include "marlin_server.hpp"
#include "bsod.h"
#include "safe_state.h"
#include "crc32.h"
#include "fusb303.hpp"
#include <crash_dump/dump.hpp>
#include "hwio_pindef.h"
#include <Arduino.h>
#include "trinamic.h"
#include "../Marlin/src/module/configuration_store.h"
#include "main.h"
#include "config_buddy_2209_02.h"
#include "timing.h"
#include "tusb.h"
#include "tasks.hpp"
#include "Marlin/src/module/planner.h"
#include <option/filament_sensor.h>

#include "filament_sensors_handler.hpp"

#if BOARD_IS_XLBUDDY
    #include <puppies/Dwarf.hpp>
    #include <Marlin/src/module/prusa/toolchanger.h>
#endif

#include <option/has_loadcell.h>
#include <option/has_loadcell_hx717.h>
#include <option/has_gui.h>

#if HAS_LOADCELL()
    #include "loadcell.h"
#endif

#if HAS_LOADCELL_HX717()
    #include "hx717.h"
#endif

LOG_COMPONENT_REF(MMU2);
LOG_COMPONENT_REF(Marlin);

#if (BOARD_IS_XBUDDY || BOARD_IS_XLBUDDY)
    #include "FUSB302B.hpp"
#endif

#if ENABLED(POWER_PANIC)
    #include "power_panic.hpp"
#endif

#include "probe_position_lookback.hpp"
#include <configuration_store.hpp>

LOG_COMPONENT_DEF(Buddy, LOG_SEVERITY_DEBUG);
LOG_COMPONENT_DEF(Core, LOG_SEVERITY_INFO);
LOG_COMPONENT_DEF(MMU2, LOG_SEVERITY_INFO);

extern void reset_trinamic_drivers();

extern "C" {
metric_t metric_app_start = METRIC("app_start", METRIC_VALUE_EVENT, 0, METRIC_HANDLER_ENABLE_ALL);
metric_t metric_maintask_event = METRIC("maintask_loop", METRIC_VALUE_EVENT, 0, METRIC_HANDLER_DISABLE_ALL);
metric_t metric_cpu_usage = METRIC("cpu_usage", METRIC_VALUE_INTEGER, 1000, METRIC_HANDLER_ENABLE_ALL);

#ifdef BUDDY_ENABLE_ETHERNET
extern osThreadId webServerTaskHandle; // Webserver thread(used for fast boot mode)
#endif                                 // BUDDY_ENABLE_ETHERNET

void app_marlin_serial_output_write_hook(const uint8_t *buffer, int size) {
    while (size && (buffer[size - 1] == '\n' || buffer[size - 1] == '\r'))
        size--;
    log_severity_t severity = LOG_SEVERITY_INFO;
    bool MMU = false;
    if (size == 2 && memcmp("ok", buffer, 2) == 0) {
        // Do not log "ok" messages
        return;
    } else if (size >= 10 && memcmp("echo:MMU2:", buffer, 10) == 0) { //@@TODO this is ugly and suboptimal
        buffer = buffer + 10;
        size -= 10;
        MMU = true;
    } else if (size >= 5 && memcmp("echo:", buffer, 5) == 0) {
        buffer = buffer + 5;
        size -= 5;
    } else if (size >= 11 && memcmp("Error:MMU2:", buffer, 11) == 0) { //@@TODO this is ugly and suboptimal
        buffer = buffer + 11;
        size -= 11;
        severity = LOG_SEVERITY_ERROR;
        MMU = true;
    } else if (size >= 6 && memcmp("Error:", buffer, 6) == 0) {
        buffer = buffer + 6;
        size -= 6;
        severity = LOG_SEVERITY_ERROR;
    }
    if (MMU) {
        log_event(severity, MMU2, "%.*s", size, buffer);
    } else {
        log_event(severity, Marlin, "%.*s", size, buffer);
    }
}

void app_setup_marlin_logging() {
    SerialUSB.lineBufferHook = app_marlin_serial_output_write_hook;
}

void app_startup() {
    app_setup_marlin_logging();
    log_info(Buddy, "marlin task waiting for dependencies");
    TaskDeps::wait(TaskDeps::Tasks::default_start);
    log_info(Buddy, "marlin task is starting");
}

void app_setup(void) {
    metric_record_event(&metric_app_start);

    if (INIT_TRINAMIC_FROM_MARLIN_ONLY == 0) {
        init_tmc();
    } else {
        init_tmc_bare_minimum();
    }

#if HAS_LOADCELL()
    loadcell.SetScale(config_store().loadcell_scale.get());
    loadcell.SetThreshold(config_store().loadcell_threshold_static.get(), Loadcell::TareMode::Static);
    loadcell.SetThreshold(config_store().loadcell_threshold_continuous.get(), Loadcell::TareMode::Continuous);
    loadcell.SetHysteresis(config_store().loadcell_hysteresis.get());
    loadcell.ConfigureSignalEvent(osThreadGetId(), 0x0A);
#endif

    setup();

    marlin_server::settings_load(); // load marlin variables from eeprom

#if (BOARD_IS_XBUDDY || BOARD_IS_XLBUDDY)
    buddy::hw::FUSB302B::ResetChip();
    buddy::hw::FUSB302B::InitChip();
#endif
}

#if (BOARD_IS_XBUDDY || BOARD_IS_XLBUDDY)
bool USBCDC_need_reset = false;

enum class reset_usb_fs_t {
    IDLE,
    START,
    END
};

enum class VBUS_state {
    not_detected = 0x00,
    detected = 0x80
};

reset_usb_fs_t reset_usb_fs = reset_usb_fs_t::IDLE;

void check_usbc_connection() {
    static uint32_t last_USBC_update = 0;
    if (ticks_s() - last_USBC_update > 1) {
        switch ((VBUS_state)buddy::hw::FUSB302B::ReadSTATUS0Reg()) {
        case VBUS_state::not_detected: // VBUS not detected
            reset_usb_fs = reset_usb_fs_t::START;
            break;
        case VBUS_state::detected: // VBUS detected
            break;
        }

        switch (reset_usb_fs) {
        case reset_usb_fs_t::IDLE:
            break;
        case reset_usb_fs_t::START:
            tud_disconnect();
            reset_usb_fs = reset_usb_fs_t::END;
            return;
        case reset_usb_fs_t::END:
            tud_connect();
            reset_usb_fs = reset_usb_fs_t::IDLE;
            break;
        }

        last_USBC_update = ticks_s();
    }
}
#endif
void app_idle(void) {
    buddy::metrics::RecordMarlinVariables();
    buddy::metrics::RecordRuntimeStats();
    buddy::metrics::RecordPrintFilename();
#if (BOARD_IS_XLBUDDY)
    buddy::metrics::record_dwarf_mcu_temperature();
#endif
    print_utils_loop();
}

void app_run(void) {
    LangEEPROM::getInstance();

    marlin_server::init();
    marlin_server::idle_cb = app_idle;

    log_info(Marlin, "Starting setup");

    app_setup();

    marlin_server::start_processing();

#if defined(HAS_ADVANCED_POWER)
    advancedpower.ResetOvercurrentFault();
#endif

    log_info(Marlin, "Setup complete");

    if (config_store_init_result() == eeprom_journal::InitResult::cold_start && marlin_server::processing()) {
        settings.reset();
#if ENABLED(POWER_PANIC)
        power_panic::reset();
#endif
    }

    TaskDeps::provide(TaskDeps::Dependency::default_task_ready);

    while (1) {
        metric_record_event(&metric_maintask_event);
        metric_record_integer(&metric_cpu_usage, osGetCPUUsage());
        if (marlin_server::processing()) {
            loop();
        }
        marlin_server::loop();
#if (BOARD_IS_XBUDDY || BOARD_IS_XLBUDDY)
        check_usbc_connection();
#endif
    }
}

void app_error(void) {
    bsod("app_error");
}

void app_assert([[maybe_unused]] uint8_t *file, [[maybe_unused]] uint32_t line) {
    bsod("app_assert");
}

#if HAS_LOADCELL_HX717()

// HX717 sample function. Sample both HX channels
static void hx717_irq() {
    if (!hx717.IsValueReady())
        return;

    static int sample_counter = 0;
    int32_t raw_value;

    static HX717::Channel current_channel = hx717.CHANNEL_A_GAIN_128;
    HX717::Channel next_channel;

    sample_counter += 1;

    if (!(loadcell.IsHighPrecisionEnabled()) && sample_counter % 13 == 0) {
        next_channel = hx717.CHANNEL_B_GAIN_8;
    } else {
        next_channel = hx717.CHANNEL_A_GAIN_128;
    }

    raw_value = hx717.ReadValue(next_channel);

    if (current_channel == hx717.CHANNEL_A_GAIN_128) {
        auto sampleRate = hx717.GetSampleRate();
        if (!std::isnan(sampleRate))
            loadcell.analysis.SetSamplingIntervalMs(sampleRate);
        uint32_t ts_ms = hx717.GetSampleTimestamp();
        loadcell.ProcessSample(raw_value, ts_ms * 1000);
    } else {
        fs_process_sample(raw_value, 0);
    }
    current_channel = next_channel;
}
#endif // HAS_LOADCELL_HX717()

#ifdef HAS_ADVANCED_POWER
static uint8_t cnt_advanced_power_update = 0;

void advanced_power_irq() {
    if (++cnt_advanced_power_update >= 40) { // update Advanced power variables = 25Hz
        advancedpower.Update();
        buddy::metrics::RecordPowerStats();
    #ifdef ADC_MULTIPLEXER
        PowerHWIDAndTempMux.switch_channel();
    #endif
        cnt_advanced_power_update = 0;
    }
}
#endif // #ifdef HAS_ADVANCED_POWER

#if (BOARD_IS_XLBUDDY && FILAMENT_SENSOR_IS_ADC())
// update filament sensor irq = 76Hz
static void filament_sensor_irq() {

    static uint8_t cnt_filament_sensor_update = 0;

    if (++cnt_filament_sensor_update >= 13) {
        for (buddy::puppies::Dwarf &dwarf : buddy::puppies::dwarfs) {
            if (!dwarf.is_enabled()) {
                continue;
            }

            // Main filament sensor
            fs_process_sample(dwarf.get_tool_filament_sensor(), dwarf.get_dwarf_nr() - 1);

            // Side filament sensor
            auto side_sensor_chanel = AdcChannel::sfs1;
            switch (dwarf.get_dwarf_nr()) {
            case 1:
                side_sensor_chanel = AdcChannel::sfs1;
                break;
            case 2:
                side_sensor_chanel = AdcChannel::sfs2;
                break;
            case 3:
                side_sensor_chanel = AdcChannel::sfs3;
                break;
            case 4:
                side_sensor_chanel = AdcChannel::sfs6; // not a bug, 6 is swapped with 4
                break;
            case 5:
                side_sensor_chanel = AdcChannel::sfs5;
                break;
            case 6:
                side_sensor_chanel = AdcChannel::sfs4; // not a bug, 6 is swapped with 4
                break;
            }
            side_fs_process_sample(AdcGet::side_filament_sensor(side_sensor_chanel), dwarf.get_dwarf_nr() - 1);
        }
        cnt_filament_sensor_update = 0;
    }
}
#endif

void adc_tick_1ms(void) {
#if HAS_LOADCELL_HX717()
    hx717_irq();
#endif // HAS_LOADCELL_HX717()

#ifdef HAS_ADVANCED_POWER
    advanced_power_irq();
#endif

#ifdef ADC_MULTIPLEXER
    SFSAndTempMux.switch_channel();
#endif

    buddy::probePositionLookback.update(planner.get_axis_position_mm(AxisEnum::Z_AXIS));
}

void app_tim14_tick(void) {
    Fans::tick();

#if HAS_GUI()
    jogwheel.Update1msFromISR();
#endif
    Sound_Update1ms();
    // hwio_update_1ms();
    adc_tick_1ms();

#if (BOARD_IS_XLBUDDY && FILAMENT_SENSOR_IS_ADC())
    filament_sensor_irq();
#endif
}

} // extern "C"

// cpp code
