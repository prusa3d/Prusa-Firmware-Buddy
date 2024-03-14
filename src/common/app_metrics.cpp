#include <inttypes.h>
#include "app_metrics.h"
#include "metric.h"
#include "log.h"
#include "version.h"
#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "malloc.h"
#include "heap.h"
#include <adc.hpp>
#include <option/has_advanced_power.h>
#if HAS_ADVANCED_POWER()
    #include "advanced_power.hpp"
#endif // HAS_ADVANCED_POWER()
#include "media.h"
#include "timing.h"
#include "config_buddy_2209_02.h"
#include "otp.hpp"
#include <array>
#include "filament.hpp"
#include "marlin_vars.hpp"
#include "config_features.h"
#include <option/has_mmu2.h>

#include "../Marlin/src/module/temperature.h"
#include "../Marlin/src/module/planner.h"
#include "../Marlin/src/module/stepper.h"
#include "../Marlin/src/feature/power.h"

#include <config_store/store_instance.hpp>

#if BOARD_IS_XLBUDDY
    #include <puppies/Dwarf.hpp>
    #include <Marlin/src/module/prusa/toolchanger.h>
#endif

#if ENABLED(PRUSA_TOOLCHANGER)
    // Loop through existing extruders
    #define FOREACH_EXTRUDER()                  \
        for (int e = 0; e < EXTRUDERS - 1; e++) \
            if (buddy::puppies::dwarfs[e].is_enabled())
    #define active_extruder_or_first active_extruder
#else
    #define FOREACH_EXTRUDER()       for (int e = 0; e < 1; e++)
    #define active_extruder_or_first 0
#endif

LOG_COMPONENT_REF(Metrics);

/// This metric is defined in Marlin/src/module/probe.cpp, thus no interface
#if HAS_BED_PROBE
extern metric_t metric_probe_z;
extern metric_t metric_probe_z_diff;
extern metric_t metric_home_diff;
#endif

void buddy::metrics::RecordRuntimeStats() {
    static metric_t fw_version = METRIC("fw_version", METRIC_VALUE_STRING, 10 * 1000, METRIC_HANDLER_ENABLE_ALL);
    metric_record_string(&fw_version, "%s", project_version_full);

    static metric_t buddy_revision = METRIC("buddy_revision", METRIC_VALUE_STRING, 10 * 10002, METRIC_HANDLER_ENABLE_ALL);
    if (metric_record_is_due(&buddy_revision)) {
        metric_record_string(&buddy_revision, "%u", otp_get_board_revision().value_or(0));
    }

    static metric_t buddy_bom = METRIC("buddy_bom", METRIC_VALUE_STRING, 10 * 10003, METRIC_HANDLER_ENABLE_ALL);
    if (metric_record_is_due(&buddy_bom)) {
        metric_record_string(&buddy_bom, "%u", otp_get_bom_id().value_or(0));
    }

    static metric_t current_filamnet = METRIC("filament", METRIC_VALUE_STRING, 10 * 1007, METRIC_HANDLER_ENABLE_ALL);
    auto current_filament = config_store().get_filament_type(marlin_vars()->active_extruder);
    metric_record_string(&current_filamnet, "%s", filament::get_description(current_filament).name);

    static metric_t stack = METRIC("stack", METRIC_VALUE_CUSTOM, 0, METRIC_HANDLER_ENABLE_ALL); // Thread stack usage
    static metric_t runtime = METRIC("runtime", METRIC_VALUE_CUSTOM, 0, METRIC_HANDLER_ENABLE_ALL); // Thread runtime usage
    constexpr const uint32_t STACK_RUNTIME_RECORD_INTERVAL_MS = 3000; // Sample stack and runtime this often
    static auto should_record_stack_runtime = RunApproxEvery(STACK_RUNTIME_RECORD_INTERVAL_MS);
    if (should_record_stack_runtime()) {
        static TaskStatus_t task_statuses[17] = {};

#if configGENERATE_RUN_TIME_STATS
        // Runtime since last record
        static uint32_t last_totaltime = 0;
        uint32_t totaltime = ticks_ms();
        uint32_t delta_totaltime = totaltime - last_totaltime;
        last_totaltime = totaltime;
        // t / 100 for percentage calculations
        // Compensate t * 1000 * TIM_BASE_CLK_MHZ to get from ms to portGET_RUN_TIME_COUNTER_VALUE() that uses TICK_TIMER
        delta_totaltime = 10UL * TIM_BASE_CLK_MHZ * delta_totaltime;

        // Last runtime of all threads to get delta later
        uint32_t last_runtime[20] = {};
        for (size_t idx = 0; idx < std::size(task_statuses); idx++) {
            if ((task_statuses[idx].xTaskNumber > 0) && (task_statuses[idx].xTaskNumber <= std::size(last_runtime))) {
                last_runtime[task_statuses[idx].xTaskNumber - 1] = task_statuses[idx].ulRunTimeCounter;
            }
        }
#endif /*configGENERATE_RUN_TIME_STATS*/

        // Get stack and runtime stats
        int count = uxTaskGetSystemState(task_statuses, std::size(task_statuses), NULL);
        if (count == 0) {
            log_error(Metrics, "Failed to record stack & runtime metrics. The task_statuses array might be too small.");
        } else {
            for (int idx = 0; idx < count; idx++) {
                // Sanitize task name
                const char *task_name = task_statuses[idx].pcTaskName;
                if (strcmp(task_name, "Tmr Svc") == 0) {
                    task_name = "TmrSvc";
                }

                // Report stack usage
                const char *stack_base = (char *)task_statuses[idx].pxStackBase;
                size_t s = 0;
                /* We can only report free stack space for heap-allocated stack frames. */
                if (mem_is_heap_allocated(stack_base)) {
                    s = malloc_usable_size((void *)stack_base);
                }
                metric_record_custom(&stack, ",n=%.7s t=%i,m=%hu", task_name, s, task_statuses[idx].usStackHighWaterMark);

#if configGENERATE_RUN_TIME_STATS
                // Report runtime usage, runtime can overflow and the difference still be valid
                if (task_statuses[idx].xTaskNumber <= std::size(last_runtime)) {
                    const uint32_t runtime_percent = (task_statuses[idx].ulRunTimeCounter - last_runtime[task_statuses[idx].xTaskNumber - 1]) / delta_totaltime;
                    metric_record_custom(&runtime, ",n=%.7s u=%u", task_name, runtime_percent);
                } else {
                    log_error(Metrics, "Failed to record runtime metric. The last_runtime array might be too small.");
                }
#endif /*configGENERATE_RUN_TIME_STATS*/
            }
        }
    }

    static metric_t heap = METRIC("heap", METRIC_VALUE_CUSTOM, 503, METRIC_HANDLER_ENABLE_ALL);
    metric_record_custom(&heap, " free=%ii,total=%ii", xPortGetFreeHeapSize(), heap_total_size);
}

void buddy::metrics::RecordMarlinVariables() {
#if HAS_BED_PROBE
    metric_register(&metric_probe_z);
    metric_register(&metric_probe_z_diff);
    metric_register(&metric_home_diff);
#endif
    static metric_t is_printing = METRIC("is_printing", METRIC_VALUE_INTEGER, 5000, METRIC_HANDLER_ENABLE_ALL);
    metric_record_integer(&is_printing, printingIsActive() ? 1 : 0);

#if ENABLED(PRUSA_TOOLCHANGER)
    static metric_t active_extruder_metric = METRIC("active_extruder", METRIC_VALUE_INTEGER, 1000, METRIC_HANDLER_ENABLE_ALL);
    metric_record_integer(&active_extruder_metric, active_extruder);
#endif

#if HAS_TEMP_HEATBREAK
    static metric_t heatbreak = METRIC("temp_hbr", METRIC_VALUE_CUSTOM, 0, METRIC_HANDLER_DISABLE_ALL); // float value, tag "n": extruder index, tag "a": is active extruder
    static auto heatbreak_should_record = RunApproxEvery(1000);
    if (heatbreak_should_record()) {
        FOREACH_EXTRUDER() {
            metric_record_custom(&heatbreak, ",n=%i,a=%i value=%.2f", e, e == active_extruder_or_first, static_cast<double>(thermalManager.degHeatbreak(e)));
        }
    }
#endif

#if HAS_TEMP_BOARD
    static metric_t board
        = METRIC("temp_brd", METRIC_VALUE_FLOAT, 1000 - 9, METRIC_HANDLER_DISABLE_ALL);
    metric_record_float(&board, thermalManager.degBoard());
#endif

#if HAS_TEMP_CHAMBER
    static metric_t chamber
        = METRIC("temp_chamber", METRIC_VALUE_FLOAT, 1000 - 10, METRIC_HANDLER_DISABLE_ALL);
    metric_record_float(&chamber, thermalManager.degChamber());
#endif /*HAS_TEMP_CHAMBER*/

    // These temperature metrics go outside of Marlin and are filtered and converted here
    static auto filtered_should_run = RunApproxEvery(1000 / OVERSAMPLENR);
    if (filtered_should_run()) {
        static uint8_t sample_nr = 0;

        static metric_t mcu = METRIC("temp_mcu", METRIC_VALUE_INTEGER, 0, METRIC_HANDLER_DISABLE_ALL);
        static int32_t mcu_sum = 0;
        mcu_sum += AdcGet::getMCUTemp();

#if BOARD_IS_XLBUDDY
        static metric_t sandwich = METRIC("temp_sandwich", METRIC_VALUE_FLOAT, 1000 - 10, METRIC_HANDLER_DISABLE_ALL);
        static int sandwich_sum = 0;
        sandwich_sum += AdcGet::sandwichTemp();

        static metric_t splitter = METRIC("temp_splitter", METRIC_VALUE_FLOAT, 1000 - 11, METRIC_HANDLER_DISABLE_ALL);
        static int splitter_sum = 0;
        splitter_sum += AdcGet::splitterTemp();
#endif /*BOARD_IS_XLBUDDY*/

        if (++sample_nr >= OVERSAMPLENR) {
            metric_record_integer(&mcu, mcu_sum / OVERSAMPLENR);
            mcu_sum = 0;
#if BOARD_IS_XLBUDDY
            // The same thermistor, use the same conversion as TEMP_BOARD
            // The function takes downsampled ADC value multiplied by OVERSAMPLENR
            metric_record_float(&sandwich, Temperature::analog_to_celsius_board(sandwich_sum));
            sandwich_sum = 0;
            if (prusa_toolchanger.is_splitter_enabled()) {
                metric_record_float(&splitter, Temperature::analog_to_celsius_board(splitter_sum));
            }
            splitter_sum = 0;
#endif /*BOARD_IS_XLBUDDY*/
            sample_nr = 0;
        }
    }

    static metric_t bed = METRIC("temp_bed", METRIC_VALUE_FLOAT, 2000 + 23, METRIC_HANDLER_DISABLE_ALL);
    metric_record_float(&bed, thermalManager.degBed());

    static metric_t target_bed = METRIC("ttemp_bed", METRIC_VALUE_INTEGER, 1000, METRIC_HANDLER_DISABLE_ALL);
    metric_record_integer(&target_bed, thermalManager.degTargetBed());

    static metric_t nozzle = METRIC("temp_noz", METRIC_VALUE_CUSTOM, 0, METRIC_HANDLER_DISABLE_ALL);
    static auto nozzle_should_record = RunApproxEvery(1000 - 10);
    if (nozzle_should_record()) {
        FOREACH_EXTRUDER() {
            metric_record_custom(&nozzle, ",n=%i,a=%i value=%.2f", e, e == active_extruder, static_cast<double>(thermalManager.degHotend(e)));
        }
    }

    static metric_t target_nozzle = METRIC("ttemp_noz", METRIC_VALUE_CUSTOM, 0, METRIC_HANDLER_DISABLE_ALL);
    static auto target_nozzle_should_record = RunApproxEvery(1000 + 9);
    if (target_nozzle_should_record()) {
        FOREACH_EXTRUDER() {
            metric_record_custom(&target_nozzle, ",n=%i,a=%i value=%ii", e, e == active_extruder, thermalManager.degTargetHotend(e));
        }
    }

#if FAN_COUNT >= 1
    static metric_t fan_speed = METRIC("fan_speed", METRIC_VALUE_INTEGER, 501, METRIC_HANDLER_DISABLE_ALL);
    metric_record_integer(&fan_speed, thermalManager.fan_speed[0]);
#endif

#if FAN_COUNT >= 2
    static metric_t heatbreak_fan_speed = METRIC("fan_hbr_speed", METRIC_VALUE_INTEGER, 502, METRIC_HANDLER_DISABLE_ALL);
    metric_record_integer(&heatbreak_fan_speed, thermalManager.fan_speed[1]);
#endif

    static metric_t ipos_x = METRIC("ipos_x", METRIC_VALUE_INTEGER, 10, METRIC_HANDLER_DISABLE_ALL);
    metric_record_integer(&ipos_x, stepper.position_from_startup(AxisEnum::X_AXIS));
    static metric_t ipos_y = METRIC("ipos_y", METRIC_VALUE_INTEGER, 10, METRIC_HANDLER_DISABLE_ALL);
    metric_record_integer(&ipos_y, stepper.position_from_startup(AxisEnum::Y_AXIS));
    static metric_t ipos_z = METRIC("ipos_z", METRIC_VALUE_INTEGER, 10, METRIC_HANDLER_DISABLE_ALL);
    metric_record_integer(&ipos_z, stepper.position_from_startup(AxisEnum::Z_AXIS));

    xyz_pos_t pos;
    planner.get_axis_position_mm(pos);
    static metric_t pos_x = METRIC("pos_x", METRIC_VALUE_FLOAT, 11, METRIC_HANDLER_DISABLE_ALL);
    metric_record_float(&pos_x, pos[X_AXIS]);
    static metric_t pos_y = METRIC("pos_y", METRIC_VALUE_FLOAT, 11, METRIC_HANDLER_DISABLE_ALL);
    metric_record_float(&pos_y, pos[Y_AXIS]);
    static metric_t pos_z = METRIC("pos_z", METRIC_VALUE_FLOAT, 11, METRIC_HANDLER_DISABLE_ALL);
    metric_record_float(&pos_z, pos[Z_AXIS]);

#if HAS_BED_PROBE
    static metric_t adj_z = METRIC("adj_z", METRIC_VALUE_FLOAT, 1500, METRIC_HANDLER_ENABLE_ALL);
    metric_record_float(&adj_z, probe_offset.z);
#endif

#if ENABLED(AUTO_POWER_CONTROL)
    static metric_t heater_enabled = METRIC("heater_enabled", METRIC_VALUE_INTEGER, 1500, METRIC_HANDLER_ENABLE_ALL);
    metric_record_integer(&heater_enabled, powerManager.is_power_needed());
#endif
}

#if HAS_ADVANCED_POWER()
    #if BOARD_IS_XBUDDY
void buddy::metrics::RecordPowerStats() {
    static metric_t metric_bed_v_raw = METRIC("volt_bed_raw", METRIC_VALUE_INTEGER, 1000, METRIC_HANDLER_DISABLE_ALL);
    metric_record_integer(&metric_bed_v_raw, advancedpower.GetBedVoltageRaw());
    static metric_t metric_bed_v = METRIC("volt_bed", METRIC_VALUE_FLOAT, 1001, METRIC_HANDLER_ENABLE_ALL);
    metric_record_float(&metric_bed_v, advancedpower.GetBedVoltage());
    static metric_t metric_nozzle_v_raw = METRIC("volt_nozz_raw", METRIC_VALUE_INTEGER, 1002, METRIC_HANDLER_DISABLE_ALL);
    metric_record_integer(&metric_nozzle_v_raw, advancedpower.GetHeaterVoltageRaw());
    static metric_t metric_nozzle_v = METRIC("volt_nozz", METRIC_VALUE_FLOAT, 1003, METRIC_HANDLER_ENABLE_ALL);
    metric_record_float(&metric_nozzle_v, advancedpower.GetHeaterVoltage());
    static metric_t metric_nozzle_i_raw = METRIC("curr_nozz_raw", METRIC_VALUE_INTEGER, 1004, METRIC_HANDLER_DISABLE_ALL);
    metric_record_integer(&metric_nozzle_i_raw, advancedpower.GetHeaterCurrentRaw());
    static metric_t metric_nozzle_i = METRIC("curr_nozz", METRIC_VALUE_FLOAT, 1005, METRIC_HANDLER_ENABLE_ALL);
    metric_record_float(&metric_nozzle_i, advancedpower.GetHeaterCurrent());
    static metric_t metric_input_i_raw = METRIC("curr_inp_raw", METRIC_VALUE_INTEGER, 1006, METRIC_HANDLER_DISABLE_ALL);
    metric_record_integer(&metric_input_i_raw, advancedpower.GetInputCurrentRaw());
    static metric_t metric_input_i = METRIC("curr_inp", METRIC_VALUE_FLOAT, 1007, METRIC_HANDLER_ENABLE_ALL);
    metric_record_float(&metric_input_i, advancedpower.GetInputCurrent());
        #if HAS_MMU2()
    static metric_t metric_mmu_i = METRIC("cur_mmu_imp", METRIC_VALUE_FLOAT, 1008, METRIC_HANDLER_ENABLE_ALL);
    metric_record_float(&metric_mmu_i, advancedpower.GetMMUInputCurrent());
        #endif
    static metric_t metric_oc_nozzle_fault = METRIC("oc_nozz", METRIC_VALUE_INTEGER, 1010, METRIC_HANDLER_ENABLE_ALL);
    metric_record_integer(&metric_oc_nozzle_fault, advancedpower.HeaterOvercurentFaultDetected());
    static metric_t metric_oc_input_fault = METRIC("oc_inp", METRIC_VALUE_INTEGER, 1011, METRIC_HANDLER_ENABLE_ALL);
    metric_record_integer(&metric_oc_input_fault, advancedpower.OvercurrentFaultDetected());
}
    #elif BOARD_IS_XLBUDDY
void buddy::metrics::RecordPowerStats() {
    static metric_t metric_splitter_5V_current = METRIC("splitter_5V_current", METRIC_VALUE_FLOAT, 1000, METRIC_HANDLER_ENABLE_ALL);
    metric_record_float(&metric_splitter_5V_current, advancedpower.GetDwarfSplitter5VCurrent());

    static metric_t metric_24VVoltage = METRIC("24VVoltage", METRIC_VALUE_FLOAT, 1001, METRIC_HANDLER_ENABLE_ALL);
    metric_record_float(&metric_24VVoltage, advancedpower.Get24VVoltage());

    static metric_t metric_5VVoltage = METRIC("5VVoltage", METRIC_VALUE_FLOAT, 1002, METRIC_HANDLER_ENABLE_ALL);
    metric_record_float(&metric_5VVoltage, advancedpower.Get5VVoltage());

    static metric_t metric_Sandwitch5VCurrent = METRIC("Sandwitch5VCurrent", METRIC_VALUE_FLOAT, 1003, METRIC_HANDLER_ENABLE_ALL);
    metric_record_float(&metric_Sandwitch5VCurrent, advancedpower.GetDwarfSandwitch5VCurrent());

    static metric_t metric_xlbuddy5VCurrent = METRIC("xlbuddy5VCurrent", METRIC_VALUE_FLOAT, 1004, METRIC_HANDLER_ENABLE_ALL);
    metric_record_float(&metric_xlbuddy5VCurrent, advancedpower.GetXLBuddy5VCurrent());
}
    #else
        #error "This board doesn't support ADVANCED_POWER"
    #endif

#endif // HAS_ADVANCED_POWER()

void buddy::metrics::RecordPrintFilename() {
    static metric_t file_name = METRIC("print_filename", METRIC_VALUE_STRING, 5000, METRIC_HANDLER_ENABLE_ALL);
    if (media_print_get_state() != media_print_state_t::media_print_state_NONE) {
        // The docstring for media_print_filename() advises against using this function; however, there is currently no replacement for it.
        metric_record_string(&file_name, "%s", marlin_vars()->media_LFN.get_ptr());
    } else {
        metric_record_string(&file_name, "");
    }
}

#if BOARD_IS_XLBUDDY
void buddy::metrics::record_dwarf_internal_temperatures() {
    // Dwarf board and MCU temperature for sensor screen
    buddy::puppies::Dwarf &dwarf = prusa_toolchanger.getActiveToolOrFirst();

    static metric_t metric_dwarfBoardTemperature = METRIC("dwarf_board_temp", METRIC_VALUE_INTEGER, 1001, METRIC_HANDLER_ENABLE_ALL);
    metric_record_integer(&metric_dwarfBoardTemperature, dwarf.get_board_temperature());

    static metric_t metric_dwarfMCUTemperature = METRIC("dwarf_mcu_temp", METRIC_VALUE_INTEGER, 1001, METRIC_HANDLER_ENABLE_ALL);
    metric_record_integer(&metric_dwarfMCUTemperature, dwarf.get_mcu_temperature());

    // All MCU temperatures
    static metric_t mcu = METRIC("dwarfs_mcu_temp", METRIC_VALUE_CUSTOM, 0, METRIC_HANDLER_DISABLE_ALL); // float value, tag "n": extruder index, tag "a": is active extruder
    static auto mcu_should_record = RunApproxEvery(1002);
    if (mcu_should_record()) {
        FOREACH_EXTRUDER() {
            metric_record_custom(&mcu, ",n=%i,a=%i value=%i", e, e == active_extruder_or_first, static_cast<int>(buddy::puppies::dwarfs[e].get_mcu_temperature()));
        }
    }

    // All board temperatures
    static metric_t board = METRIC("dwarfs_board_temp", METRIC_VALUE_CUSTOM, 0, METRIC_HANDLER_DISABLE_ALL); // float value, tag "n": extruder index, tag "a": is active extruder
    static auto board_should_record = RunApproxEvery(1003);
    if (board_should_record()) {
        FOREACH_EXTRUDER() {
            metric_record_custom(&board, ",n=%i,a=%i value=%i", e, e == active_extruder_or_first, static_cast<int>(buddy::puppies::dwarfs[e].get_board_temperature()));
        }
    }
}
#endif
