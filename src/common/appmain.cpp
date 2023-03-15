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
#include "eeprom.h"
#include "safe_state.h"
#include "crc32.h"
#include "fusb303.hpp"
#include <crash_dump/dump.h>
#include "hwio_pindef.h"
#include <Arduino.h>
#include "trinamic.h"
#include "../Marlin/src/module/configuration_store.h"
#include "main.h"
#include "config_buddy_2209_02.h"
#include "timing.h"
#include "tusb.h"
#include "tasks.h"
#include "Marlin/src/module/planner.h"
#include <option/filament_sensor.h>

#include "filament_sensors_handler.hpp"

#if BOARD_IS_XLBUDDY
    #include <puppies/Dwarf.hpp>
    #include <Marlin/src/module/prusa/toolchanger.h>
#endif

#include <option/has_loadcell.h>
#include <option/has_gui.h>

#if HAS_LOADCELL()
    #include "loadcell.h"
#endif

#ifdef LOADCELL_HX717
    #include "hx717.h"
#endif //LOADCELL_HX717

LOG_COMPONENT_REF(MMU2);
LOG_COMPONENT_REF(Marlin);

#if (BOARD_IS_XBUDDY || BOARD_IS_XLBUDDY)
    #include "FUSB302B.hpp"
#endif

#if (BOARD_IS_XBUDDY && defined LOVEBOARD_HAS_EEPROM)
    #include "calibrated_loveboard.hpp"
#endif

#ifdef HAS_ACCELEROMETR
    #include "SparkFunLIS3DH.h"
#endif

#if ENABLED(POWER_PANIC)
    #include "power_panic.hpp"
#endif

#include "probe_position_lookback.hpp"

#if (BOARD_IS_XBUDDY && defined LOVEBOARD_HAS_EEPROM)
CalibratedLoveboard *LoveBoard;
#endif //(BOARD_IS_XBUDDY && defined LOVEBOARD_HAS_EEPROM)

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
#endif                                 //BUDDY_ENABLE_ETHERNET

#ifdef HAS_ACCELEROMETR
LIS3DH accelerometer(SPI_MODE, 10);
#endif

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
    log_info(Buddy, "marlin task waiting for dependecies");
    wait_for_dependecies(DEFAULT_TASK_DEPS);
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
    loadcell.SetScale(variant8_get_flt(eeprom_get_var(EEVAR_LOADCELL_SCALE)));
    loadcell.SetThreshold(variant8_get_flt(eeprom_get_var(EEVAR_LOADCELL_THRS_STATIC)), Loadcell::TareMode::Static);
    loadcell.SetThreshold(variant8_get_flt(eeprom_get_var(EEVAR_LOADCELL_THRS_CONTINOUS)), Loadcell::TareMode::Continuous);
    loadcell.SetHysteresis(variant8_get_flt(eeprom_get_var(EEVAR_LOADCELL_HYST)));
    loadcell.ConfigureSignalEvent(osThreadGetId(), 0x0A);
#endif

#if (BOARD_IS_XBUDDY && defined LOVEBOARD_HAS_EEPROM)
    #if (BOARD_IS_XBUDDY && BOARD_VER_EQUAL_TO(0, 1, 8))
    LoveBoard = new CalibratedLoveboard(GPIOE, LL_GPIO_PIN_6);
    #elif (BOARD_IS_XBUDDY && (BOARD_VER_HIGHER_OR_EQUAL_TO(0, 2, 0)))
    LoveBoard = new CalibratedLoveboard(GPIOF, LL_GPIO_PIN_13);
    #endif
#endif
    setup();

    marlin_server_settings_load(); // load marlin variables from eeprom

#ifdef HAS_ACCELEROMETR
    accelerometer.begin();
#endif

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
    Buddy::Metrics::RecordMarlinVariables();
    Buddy::Metrics::RecordRuntimeStats();
    Buddy::Metrics::RecordPrintFilename();
#if (BOARD_IS_XLBUDDY)
    Buddy::Metrics::record_dwarf_mcu_temperature();
#endif
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

#if defined(HAS_ADVANCED_POWER)
    advancedpower.ResetOvercurrentFault();
#endif

    log_info(Marlin, "Setup complete");

    if (eeprom_init() == EEPROM_INIT_Defaults && marlin_server_processing()) {
        settings.reset();
#if ENABLED(POWER_PANIC)
        power_panic::reset();
#endif
    }

    provide_dependecy(ComponentDependencies::DEFAULT_TASK_READY_IDX);

    while (1) {
        metric_record_event(&metric_maintask_event);
        metric_record_integer(&metric_cpu_usage, osGetCPUUsage());
        if (marlin_server_processing()) {
            loop();
        }
        marlin_server_loop();
#if (BOARD_IS_XBUDDY || BOARD_IS_XLBUDDY)
        check_usbc_connection();
#endif
    }
}

void app_error(void) {
    bsod("app_error");
}

void app_assert(uint8_t *file, uint32_t line) {
    bsod("app_assert");
}

#ifdef LOADCELL_HX717

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
        loadcell.ProcessSample(raw_value, ticks_us());
        auto sampleRate = hx717.GetSampleRate();
        if (!std::isnan(sampleRate))
            loadcell.analysis.SetSamplingIntervalMs(sampleRate);
    } else {
    #if (BOARD_IS_XBUDDY && defined LOVEBOARD_HAS_PT100)
        static metric_t hx717_ch_b = METRIC("ad_hx717_ch_b", METRIC_VALUE_FLOAT, 500, METRIC_HANDLER_ENABLE_ALL);
        metric_record_float(&hx717_ch_b, (float)raw_value);
        hwio_set_hotend_temp_raw(raw_value);
    #else
        fs_process_sample(raw_value, 0);
    #endif
    }
    current_channel = next_channel;
}
#endif //LOADCELL_HX717

#ifdef HAS_ADVANCED_POWER
static uint8_t cnt_advanced_power_update = 0;

void advanced_power_irq() {
    if (++cnt_advanced_power_update >= 40) { // update Advanced power variables = 25Hz
        advancedpower.Update();
        Buddy::Metrics::RecordPowerStats();
    #ifdef ADC_MULTIPLEXER
        PowerHWIDAndTempMux.switch_channel();
    #endif
        cnt_advanced_power_update = 0;
    }
}
#endif //#ifdef HAS_ADVANCED_POWER

#if ((BOARD_IS_XBUDDY && defined LOVEBOARD_HAS_PT100) || (BOARD_IS_XLBUDDY && FILAMENT_SENSOR_IS_ADC()))
// update filament sensor irq = 76Hz
static void filament_sensor_irq() {

    static uint8_t cnt_filament_sensor_update = 0;

    if (++cnt_filament_sensor_update >= 13) {
    #if (BOARD_IS_XBUDDY && defined LOVEBOARD_HAS_PT100)
        fs_process_sample(AdcGet::filamentSensor(), 0);
    #elif BOARD_IS_XLBUDDY
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
    #endif
        cnt_filament_sensor_update = 0;
    }
}
#endif

#ifdef HAS_ACCELEROMETR
static metric_t accel = METRIC("tk_accel", METRIC_VALUE_CUSTOM, 10, METRIC_HANDLER_ENABLE_ALL);

void accelerometer_irq() {
    if (accelerometer.isSetupDone() == true) {
        //_dbg("acell x = %1.4d, y = %1.4d, z = %1.4d", (double)SensorOne.readFloatAccelX(), (double)SensorOne.readFloatAccelY(), (double)SensorOne.readFloatAccelZ());
        metric_record_custom(&accel, " x=%.4f,y=%.4f,z=%.4f", (double)accelerometer.readFloatAccelX(), (double)accelerometer.readFloatAccelY(), (double)accelerometer.readFloatAccelZ());
    }
}

#endif

void adc_tick_1ms(void) {
#ifdef LOADCELL_HX717
    hx717_irq();
#endif //LOADCELL_HX717

#ifdef HAS_ADVANCED_POWER
    advanced_power_irq();
#endif

#ifdef HAS_ACCELEROMETR
    accelerometer_irq();
#endif

#ifdef ADC_MULTIPLEXER
    SFSAndTempMux.switch_channel();
#endif

    buddy::probePositionLookback.update(planner.get_axis_position_mm(AxisEnum::Z_AXIS));
}

void app_tim14_tick(void) {
    fanctl_tick();

#if HAS_GUI()
    jogwheel.Update1msFromISR();
#endif
    Sound_Update1ms();
    //hwio_update_1ms();
    adc_tick_1ms();

#if ((BOARD_IS_XBUDDY && defined LOVEBOARD_HAS_PT100) || (BOARD_IS_XLBUDDY && FILAMENT_SENSOR_IS_ADC()))
    filament_sensor_irq();
#endif
}

} // extern "C"

//cpp code
