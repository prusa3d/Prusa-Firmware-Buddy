#include "appmain.hpp"

#include "app_metrics.h"
#include <logging/log.hpp>
#include "cmsis_os.h"
#include "config.h"
#include "adc.hpp"
#include <option/has_gui.h>
#if HAS_GUI()
    #include "Jogwheel.hpp"
#endif
#include "hwio.h"
#include "sys.h"
#include "gpio.h"
#include "metric.h"
#include "cpu_utils.hpp"
#include "sound.hpp"
#include "language_eeprom.hpp"
#include <device/board.h>
#include <usb_device.hpp>

#include <option/has_advanced_power.h>
#if HAS_ADVANCED_POWER()
    #include "advanced_power.hpp"
#endif // HAS_ADVANCED_POWER()

#include "marlin_server.hpp"
#include "bsod.h"
#include "safe_state.h"
#include "crc32.h"
#include <crash_dump/dump.hpp>
#include "hwio_pindef.h"
#include <Arduino.h>
#include "trinamic.h"
#include "../Marlin/src/module/configuration_store.h"
#include "main.h"
#include <stdint.h>
#include "fanctl.hpp"
#include "printers.h"
#include "MarlinPin.h"
#include "timing.h"
#include "tasks.hpp"
#include "Marlin/src/module/planner.h"
#include <option/filament_sensor.h>

#include <tusb.h>

#if BOARD_IS_XLBUDDY()
    #include <puppies/Dwarf.hpp>
    #include <Marlin/src/module/prusa/toolchanger.h>
    #include <filament_sensors_handler.hpp>
    #include <filament_sensors_handler_XL_remap.hpp>
#endif

#include <option/has_loadcell.h>
#if HAS_LOADCELL()
    #include "loadcell.hpp"
    #include "feature/prusa/e-stall_detector.h"
#endif

#include <option/has_loadcell_hx717.h>
#if HAS_LOADCELL_HX717()
    #include "hx717mux.hpp"
#endif

#include <option/has_touch.h>
#if HAS_TOUCH()
    #include <hw/touchscreen/touchscreen.hpp>
#endif

LOG_COMPONENT_REF(MMU2);
LOG_COMPONENT_REF(Marlin);

#if ENABLED(POWER_PANIC)
    #include "power_panic.hpp"
#endif

#include "probe_position_lookback.hpp"
#include <config_store/store_instance.hpp>
#include <option/init_trinamic_from_marlin_only.h>

LOG_COMPONENT_DEF(Buddy, logging::Severity::debug);
LOG_COMPONENT_DEF(Core, logging::Severity::info);
LOG_COMPONENT_DEF(MMU2, logging::Severity::info);

METRIC_DEF(metric_app_start, "app_start", METRIC_VALUE_EVENT, 0, METRIC_ENABLED);
METRIC_DEF(metric_maintask_event, "maintask_loop", METRIC_VALUE_EVENT, 0, METRIC_DISABLED);
METRIC_DEF(metric_cpu_usage, "cpu_usage", METRIC_VALUE_INTEGER, 1000, METRIC_ENABLED);

void app_marlin_serial_output_write_hook(const uint8_t *buffer, int size) {
    while (size && (buffer[size - 1] == '\n' || buffer[size - 1] == '\r')) {
        size--;
    }
    logging::Severity severity = logging::Severity::info;
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
        severity = logging::Severity::error;
        MMU = true;
    } else if (size >= 6 && memcmp("Error:", buffer, 6) == 0) {
        buffer = buffer + 6;
        size -= 6;
        severity = logging::Severity::error;
    }
    if (MMU) {
        log_event(severity, MMU2, "%.*s", size, buffer);
    } else {
        log_event(severity, Marlin, "%.*s", size, buffer);
    }
}

static void app_setup_marlin_logging() {
    SerialUSB.lineBufferHook = app_marlin_serial_output_write_hook;
}

static void wait_for_serial() {
    // wait for usb thread to be ready, then continue waiting only if something was seen
    TaskDeps::wait(TaskDeps::Tasks::usb_device_start);
    if (!usb_device_seen()) {
        return;
    }

    // If a device was seen, keep trying to connect irregardless of the current connection state, as
    // a re-negotiation could temporarily break out of this loop a cause messages to be lost
    log_info(Buddy, "device seen: waiting for serial");
    uint32_t start_ts = ticks_ms();
    while (ticks_diff(ticks_ms(), start_ts) < 3000) {
        if (tud_cdc_n_connected(0)) {
            log_info(Buddy, "serial successfully attached");
            break;
        }
        osDelay(10);
    }
}

static void app_startup() {
    // Attempt to wait for CDC to initialize to get the full Marlin startup output
    wait_for_serial();

    // Finally link SerialUSB/marlin
    app_setup_marlin_logging();

    log_info(Buddy, "marlin task waiting for dependencies");
    TaskDeps::wait(TaskDeps::Tasks::default_start);
    log_info(Buddy, "marlin task is starting");
}

static void app_setup(void) {
    metric_record_event(&metric_app_start);

    if constexpr (!INIT_TRINAMIC_FROM_MARLIN_ONLY()) {
        init_tmc();
    } else {
        init_tmc_bare_minimum();
    }

#if HAS_LOADCELL()
    // loadcell configuration
    loadcell.SetScale(config_store().loadcell_scale.get());
    loadcell.SetThreshold(config_store().loadcell_threshold_static.get(), Loadcell::TareMode::Static);
    loadcell.SetThreshold(config_store().loadcell_threshold_continuous.get(), Loadcell::TareMode::Continuous);
    loadcell.SetHysteresis(config_store().loadcell_hysteresis.get());

    if (config_store().stuck_filament_detection.get()) {
        EMotorStallDetector::Instance().SetEnabled();
    } // else keep it disabled (which is the default)

    #if HAS_LOADCELL_HX717()
    buddy::hw::hx717mux.init();
    #endif
#endif

    setup();
}

void app_run(void) {
    app_startup();

#if HAS_GUI()
    LangEEPROM::getInstance();
#endif

    app_setup();
    marlin_server::init();

#if HAS_ADVANCED_POWER()
    advancedpower.ResetOvercurrentFault();
#endif

    if (config_store_init_result() == config_store_ns::InitResult::cold_start) {
        settings.reset();
#if ENABLED(POWER_PANIC)
        power_panic::reset();
#endif
    }

    TaskDeps::provide(TaskDeps::Dependency::default_task_ready);

    while (1) {
        metric_record_event(&metric_maintask_event);
        metric_record_integer(&metric_cpu_usage, osGetCPUUsage());
        loop();
        marlin_server::loop();
    }
}

void app_error(void) {
    bsod("app_error");
}

void app_assert([[maybe_unused]] uint8_t *file, [[maybe_unused]] uint32_t line) {
    bsod("app_assert");
}

#if HAS_ADVANCED_POWER()
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
#endif // #if HAS_ADVANCED_POWER()

#if (BOARD_IS_XLBUDDY() && FILAMENT_SENSOR_IS_ADC())
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
            auto mapping = side_fsensor_remap::get_mapping();
            assert(static_cast<size_t>(dwarf.get_dwarf_nr() - 1) < std::size(mapping));
            const uint8_t remapped = mapping[dwarf.get_dwarf_nr() - 1];
            assert(remapped < HOTENDS);

            /**
             * @brief Mapping of ADC channels to each extruder side filament sensor.
             * ADC channels are laid left top to bottom and right bottom to top.
             * Left    Right
             * sfs1    sfs6
             * sfs2    sfs5
             * sfs3    sfs4
             */
            static const constexpr std::array<AdcChannel::SideFilamnetSensorsAndTempMux, HOTENDS> adc_channel_mapping = {
                AdcChannel::SideFilamnetSensorsAndTempMux::sfs1, // T0    - left top
                AdcChannel::SideFilamnetSensorsAndTempMux::sfs2, // T1    - left middle
                AdcChannel::SideFilamnetSensorsAndTempMux::sfs3, // T2    - left bottom
                AdcChannel::SideFilamnetSensorsAndTempMux::sfs6, // T3    - right top
                AdcChannel::SideFilamnetSensorsAndTempMux::sfs5, // T4    - right middle
                AdcChannel::SideFilamnetSensorsAndTempMux::sfs4, // Empty - right bottom
            };

            // ensure AdcGet::undefined_value is representable within FSensor::value_type
            static_assert(static_cast<IFSensor::value_type>(AdcGet::undefined_value) == AdcGet::undefined_value);

            // widen the type to match the main sensor data type and translate the undefined value
            IFSensor::value_type fs_raw_value = AdcGet::side_filament_sensor(adc_channel_mapping[remapped]);
            if (fs_raw_value == AdcGet::undefined_value) {
                fs_raw_value = IFSensor::undefined_value;
            }
            side_fs_process_sample(fs_raw_value, dwarf.get_dwarf_nr() - 1);
        }
        cnt_filament_sensor_update = 0;
    }
}
#endif

void adc_tick_1ms(void) {
#if HAS_ADVANCED_POWER()
    advanced_power_irq();
#endif

#ifdef ADC_MULTIPLEXER
    SFSAndTempMux.switch_channel();
#endif

#if HAS_LOADCELL()
    buddy::probePositionLookback.update(planner.get_axis_position_mm(AxisEnum::Z_AXIS));
#endif
}

void app_tim14_tick(void) {
    // run sound first, so it is more synchronized
    Sound_Update1ms();

    Fans::tick();

#if HAS_GUI()
    jogwheel.Update1msFromISR();
#endif

#if HAS_TOUCH()
    if (touchscreen.is_enabled()) {
        touchscreen.update();
    }
#endif

    adc_tick_1ms();

#if (BOARD_IS_XLBUDDY() && FILAMENT_SENSOR_IS_ADC())
    filament_sensor_irq();
#endif
}

// cpp code
