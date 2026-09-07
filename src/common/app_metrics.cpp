#include <inttypes.h>
#include "app_metrics.h"
#include "metric.h"
#include <logging/log.hpp>
#include <common/sensor_data.hpp>
#include <version/version.hpp>
#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "malloc.h"
#include "heap.h"
#include <utils/variant_utils.hpp>
#include <adc.hpp>
#include <option/has_door_sensor.h>
#include <option/has_local_bed.h>
#include <option/has_cyphal_metrics.h>
#if HAS_CYPHAL_METRICS()
    // #error dead code found by automatic analyses (see BFW-5461)
    #include <cyphal_task.hpp>
#endif /* HAS_CYPHAL_METRICS() */
#include <option/has_advanced_power.h>
#if HAS_ADVANCED_POWER()
    #include "advanced_power.hpp"
#endif // HAS_ADVANCED_POWER()
#include "timing.h"
#include <stdint.h>
#include <device/board.h>
#include "printers.h"
#include "MarlinPin.h"
#include "otp.hpp"
#include <array>
#include "filament.hpp"
#include "marlin_vars.hpp"
#include "marlin_server.hpp"
#include "config_features.h"
#include <option/has_mmu2.h>
#include <metric_handlers.h>
#include <utils/timing/rate_limiter.hpp>
#include <utils/overloaded_visitor.hpp>

#include <Marlin/src/module/temperature.h>
#include <Marlin/src/module/planner.h>
#include <Marlin/src/module/stepper.h>
#include <Marlin/src/feature/power.h>
#include <Marlin/src/feature/phase_stepping/debug_events.hpp>

#include <config_store/store_instance.hpp>
#include <option/has_toolchanger.h>

#if BOARD_IS_XLBUDDY()
    #include <puppies/Dwarf.hpp>
    #include <Marlin/src/module/prusa/toolchanger.h>
#endif

using namespace buddy::metrics;

LOG_COMPONENT_REF(Metrics);

namespace {

void RecordRuntimeStats() {
    METRIC_DEF(fw_version, "fw_version", METRIC_VALUE_STRING, 65535, METRIC_ENABLED);
    metric_record_string(&fw_version, "%s", version::project_version_full);

    METRIC_DEF(buddy_revision, "buddy_revision", METRIC_VALUE_STRING, 65534, METRIC_ENABLED);
    if (metric_record_is_due(&buddy_revision)) {
        metric_record_string(&buddy_revision, "%u", otp_get_board_revision().value_or(0));
    }

    METRIC_DEF(buddy_bom, "buddy_bom", METRIC_VALUE_STRING, 65533, METRIC_ENABLED);
    if (metric_record_is_due(&buddy_bom)) {
        metric_record_string(&buddy_bom, "%u", otp_get_bom_id().value_or(0));
    }

    METRIC_DEF(metric_current_filament, "filament", METRIC_VALUE_STRING, 10 * 1007, METRIC_ENABLED);

    const FilamentType current_filament = match(
        marlin_vars().active_extruder.get(),
        [](VirtualToolIndex virtual_tool) -> FilamentType { return config_store().get_filament_type(virtual_tool); },
        [](NoTool) { return FilamentType::none; });
    metric_record_string(&metric_current_filament, "%s", current_filament.parameters().name.data());

    METRIC_DEF(stack, "stack", METRIC_VALUE_CUSTOM, 0, METRIC_ENABLED); // Thread stack usage
    METRIC_DEF(runtime, "runtime", METRIC_VALUE_CUSTOM, 0, METRIC_ENABLED); // Thread runtime usage
    constexpr const uint32_t STACK_RUNTIME_RECORD_INTERVAL_MS = 3000; // Sample stack and runtime this often
    static auto should_record_stack_runtime = RateLimiter<uint32_t>(STACK_RUNTIME_RECORD_INTERVAL_MS);
    if (should_record_stack_runtime.check(ticks_ms())) {
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
        uint32_t last_runtime[21] = {};
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
                const char *task_name = task_statuses[idx].pcTaskName;

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
                    metric_record_custom(&runtime, ",n=%.7s u=%" PRIu32, task_name, runtime_percent);
                } else {
                    log_error(Metrics, "Failed to record runtime metric. The last_runtime array might be too small.");
                }
#endif /*configGENERATE_RUN_TIME_STATS*/
            }
        }
    }

    METRIC_DEF(heap, "heap", METRIC_VALUE_CUSTOM, 503, METRIC_ENABLED);
    metric_record_custom(&heap, " free=%zui,total=%zui", xPortGetFreeHeapSize(), static_cast<size_t>(heap_total_size()));

    // Config store metrics (BFW-7758)
    {
        // Should be same for all store metrics (so that we can get synchronized datapoints in Grafana)
        // Should not be a "whole second" number to reduce metrics bursts when a lot of metrics get reported at the same time, overflowing buffers
        static constexpr uint32_t store_metrics_interval_ms = 5051;

        /// Cumulative count of config store migrations since printer start
        METRIC_DEF(store_migrations, "store_migrations", METRIC_VALUE_INTEGER, store_metrics_interval_ms, METRIC_ENABLED);
        metric_record_integer(&store_migrations, config_store().get_backend().bank_migration_count());

        /// Cumulative count of bytes written to EEPROM by config_store since printer start
        METRIC_DEF(store_bytes, "store_bytes", METRIC_VALUE_INTEGER, store_metrics_interval_ms, METRIC_ENABLED);
        metric_record_integer(&store_bytes, EEPROMInstance().bytes_written());

        /// Cumulative count of items written to EEPROM by config_store since printer start (outside of bank migrations)
        METRIC_DEF(store_items, "store_items", METRIC_VALUE_INTEGER, store_metrics_interval_ms, METRIC_ENABLED);
        metric_record_integer(&store_items, config_store().get_backend().item_write_count());
    }

#if HAS_CYPHAL_METRICS()
    // #error dead code found by automatic analyses (see BFW-5461)
    METRIC_DEF(can_error_log, "can_error_log", METRIC_VALUE_INTEGER, 505, METRIC_ENABLED);
    metric_record_integer(&can_error_log, can::cyphal::cyphal_task.get_error_log());
#endif /* HAS_CYPHAL_METRICS() */
}

void RecordMarlinVariables() {
    METRIC_DEF(is_printing, "is_printing", METRIC_VALUE_INTEGER, 5000, METRIC_ENABLED);
    metric_record_integer(&is_printing, printingIsActive() ? 1 : 0);

#if HAS_TOOLCHANGER()
    METRIC_DEF(active_extruder_metric, "active_extruder", METRIC_VALUE_INTEGER, 1000, METRIC_ENABLED);
    metric_record_integer(&active_extruder_metric, active_extruder);
#endif

#if HAS_TEMP_HEATBREAK
    METRIC_DEF(heatbreak, "temp_hbr", METRIC_VALUE_CUSTOM, 0, METRIC_DISABLED); // float value, tag "n": extruder index, tag "a": is active extruder
    static auto heatbreak_should_record = RateLimiter<uint32_t>(1000);
    if (heatbreak_should_record.check(ticks_ms())) {
        for (auto tool : PhysicalToolIndex::all().skip_all_disabled()) {
            metric_record_custom(&heatbreak, ",n=%i,a=%i value=%.2f", tool.to_raw(), stdext::holds_value(PhysicalToolIndex::currently_selected(), tool), static_cast<double>(thermalManager.degHeatbreak(tool)));
        }
    }
#endif

#if HAS_TEMP_BOARD
    {
        METRIC_DEF(board, "temp_brd", METRIC_VALUE_FLOAT, 1000 - 9, METRIC_DISABLED);
        metric_record_float(&board, sensor_data().boardTemp.load());
    }
#endif

    // These temperature metrics go outside of Marlin and are filtered and converted here
    {
        METRIC_DEF(mcu, "temp_mcu", METRIC_VALUE_INTEGER, 0, METRIC_DISABLED);
        metric_record_integer(&mcu, static_cast<int>(sensor_data().MCUTemp.load()));
    }

#if BOARD_IS_XLBUDDY()
    {
        METRIC_DEF(sandwich, "temp_sandwich", METRIC_VALUE_FLOAT, 1000 - 10, METRIC_DISABLED);
        metric_record_float(&sandwich, sensor_data().sandwichTemp.load());
    }
    if (prusa_toolchanger.is_splitter_enabled()) {
        METRIC_DEF(splitter, "temp_splitter", METRIC_VALUE_FLOAT, 1000 - 11, METRIC_DISABLED);
        metric_record_float(&splitter, sensor_data().splitterTemp.load());
    }
#endif

    METRIC_DEF(metric_nozzle_pwm, "nozzle_pwm", METRIC_VALUE_INTEGER, 1000, METRIC_DISABLED);
    metric_record_integer(&metric_nozzle_pwm, thermalManager.nozzle_pwm);

#if HAS_LOCAL_BED()
    METRIC_DEF(metric_bed_pwm, "bed_pwm", METRIC_VALUE_INTEGER, 1000, METRIC_DISABLED);
    metric_record_integer(&metric_bed_pwm, thermalManager.bed_pwm);
#endif

    METRIC_DEF(bed, "temp_bed", METRIC_VALUE_FLOAT, 2000 + 23, METRIC_DISABLED);
    metric_record_float(&bed, thermalManager.degBed());

    METRIC_DEF(target_bed, "ttemp_bed", METRIC_VALUE_INTEGER, 1000, METRIC_DISABLED);
    metric_record_integer(&target_bed, thermalManager.degTargetBed());

    METRIC_DEF(nozzle, "temp_noz", METRIC_VALUE_CUSTOM, 0, METRIC_DISABLED);
    static auto nozzle_should_record = RateLimiter<uint32_t>(1000 - 10);
    if (nozzle_should_record.check(ticks_ms())) {
        for (auto tool : PhysicalToolIndex::all().skip_all_disabled()) {
            metric_record_custom(&nozzle, ",n=%i,a=%i value=%.2f", tool.to_raw(), stdext::holds_value(PhysicalToolIndex::currently_selected(), tool), static_cast<double>(thermalManager.degHotend(tool)));
        }
    }

    METRIC_DEF(target_nozzle, "ttemp_noz", METRIC_VALUE_CUSTOM, 0, METRIC_DISABLED);
    static auto target_nozzle_should_record = RateLimiter<uint32_t>(1000 + 9);
    if (target_nozzle_should_record.check(ticks_ms())) {
        for (auto tool : PhysicalToolIndex::all().skip_all_disabled()) {
            metric_record_custom(&target_nozzle, ",n=%i,a=%i value=%ii", tool.to_raw(), stdext::holds_value(PhysicalToolIndex::currently_selected(), tool), thermalManager.degTargetHotend(tool));
        }
    }

#if PRINTER_IS_PRUSA_iX()
    METRIC_DEF(temp_psu, "temp_psu", METRIC_VALUE_FLOAT, 1100, METRIC_ENABLED);
    metric_record_float(&temp_psu, thermalManager.deg_psu());

    METRIC_DEF(temp_ambient, "temp_ambient", METRIC_VALUE_FLOAT, 1100, METRIC_ENABLED);
    metric_record_float(&temp_ambient, thermalManager.deg_ambient());
#endif

#if HAS_DOOR_SENSOR()
    {
        METRIC_DEF(door_sensor, "door_sensor", METRIC_VALUE_INTEGER, 1100, METRIC_ENABLED);
        metric_record_integer(&door_sensor, sensor_data().door_sensor_detailed_state.load().raw_data);
    }
#endif

    METRIC_DEF(fan_speed, "fan_speed", METRIC_VALUE_INTEGER, 501, METRIC_DISABLED);
    metric_record_integer(&fan_speed, thermalManager.print_fan_speed);

#if HAS_TEMP_HEATBREAK_CONTROL
    {
        METRIC_DEF(heatbreak_fan_speed, "fan_hbr_speed", METRIC_VALUE_INTEGER, 502, METRIC_DISABLED);
        metric_record_integer(&heatbreak_fan_speed, static_cast<int>(sensor_data().heatbreak_fan_pwm.load()));
    }
#endif

    METRIC_DEF(ipos_x, "ipos_x", METRIC_VALUE_INTEGER, 10, METRIC_DISABLED);
    metric_record_integer(&ipos_x, stepper.position_from_startup(AxisEnum::X_AXIS));
    METRIC_DEF(ipos_y, "ipos_y", METRIC_VALUE_INTEGER, 10, METRIC_DISABLED);
    metric_record_integer(&ipos_y, stepper.position_from_startup(AxisEnum::Y_AXIS));
    METRIC_DEF(ipos_z, "ipos_z", METRIC_VALUE_INTEGER, 10, METRIC_DISABLED);
    metric_record_integer(&ipos_z, stepper.position_from_startup(AxisEnum::Z_AXIS));

    xyz_pos_t pos;
    planner.get_axis_position_mm(pos);
    METRIC_DEF(pos_x, "pos_x", METRIC_VALUE_FLOAT, 11, METRIC_DISABLED);
    metric_record_float(&pos_x, pos[X_AXIS]);
    METRIC_DEF(pos_y, "pos_y", METRIC_VALUE_FLOAT, 11, METRIC_DISABLED);
    metric_record_float(&pos_y, pos[Y_AXIS]);
    METRIC_DEF(pos_z, "pos_z", METRIC_VALUE_FLOAT, 11, METRIC_DISABLED);
    metric_record_float(&pos_z, pos[Z_AXIS]);

    /// Integer that increases/changes every time a motor stall is detected - meaning the planner has run out of commands.
    /// If this is encountered during printing, it might be a cause of print artefacts
    METRIC_DEF(metric_stepper_stall, "stp_stall", METRIC_VALUE_INTEGER, 100, METRIC_ENABLED);
    metric_record_integer(&metric_stepper_stall, PreciseStepping::stall_count);

    /// Planner buffer metrics.
    /// plan_cnt: cumulative move blocks consumed (monotonic, consumer computes deltas)
    /// plan_min: minimum nonbusy_movesplanned() observed per window (reset-on-read)
    /// plan_unopt: cumulative blocks consumed before the planner marked them as
    ///             fully optimized (monotonic, consumer computes deltas)
    METRIC_DEF(metric_planner_block_count, "plan_cnt", METRIC_VALUE_INTEGER, 100, METRIC_DISABLED);
    METRIC_DEF(metric_planner_min_depth, "plan_min", METRIC_VALUE_INTEGER, 100, METRIC_DISABLED);
    METRIC_DEF(metric_planner_unoptimized, "plan_unopt", METRIC_VALUE_INTEGER, 100, METRIC_DISABLED);

    const uint32_t plan_count = PreciseStepping::planner_block_count.load(std::memory_order_relaxed);
    metric_record_integer(&metric_planner_block_count, plan_count);

    const uint8_t plan_min = PreciseStepping::planner_min_depth.exchange(UINT8_MAX, std::memory_order_relaxed);
    metric_record_integer(&metric_planner_min_depth, plan_min);

    metric_record_integer(&metric_planner_unoptimized, PreciseStepping::planner_unoptimized_count.load(std::memory_order_relaxed));

#if ENABLED(SLOWDOWN)
    /// Cumulative count of blocks where the SLOWDOWN mechanism reduced feedrate
    METRIC_DEF(metric_planner_slowdown, "plan_slow", METRIC_VALUE_INTEGER, 100, METRIC_DISABLED);
    metric_record_integer(&metric_planner_slowdown, Planner::slowdown_count.load(std::memory_order_relaxed));
#endif

    /// Position of the last executed gcode in the media stream
    METRIC_DEF(metric_sdpos, "sdpos", METRIC_VALUE_INTEGER, 100, METRIC_ENABLED);
    metric_record_integer(&metric_sdpos, marlin_vars().media_position.get());

    /// Executed gcode count since printer start
    METRIC_DEF(metric_cmdcnt, "cmdcnt", METRIC_VALUE_INTEGER, 100, METRIC_ENABLED);
    metric_record_integer(&metric_cmdcnt, GCodeQueue::executed_commmand_count);

#if HAS_BED_PROBE
    METRIC_DEF(adj_z, "adj_z", METRIC_VALUE_FLOAT, 1500, METRIC_ENABLED);
    metric_record_float(&adj_z, probe_offset.z);
#endif

#if ENABLED(AUTO_POWER_CONTROL)
    METRIC_DEF(heater_enabled, "heater_enabled", METRIC_VALUE_INTEGER, 1500, METRIC_ENABLED);
    metric_record_integer(&heater_enabled, powerManager.is_power_needed());
#endif
}

#if HAS_ADVANCED_POWER()
    #if BOARD_IS_XBUDDY()
void RecordPowerStats() {
    {
        METRIC_DEF(bed_voltage, "bed_voltage", METRIC_VALUE_FLOAT, 1001, METRIC_ENABLED);
        metric_record_float(&bed_voltage, sensor_data().bed_voltage.load());
    }
    {
        METRIC_DEF(heater_voltage, "heater_voltage", METRIC_VALUE_FLOAT, 1003, METRIC_ENABLED);
        metric_record_float(&heater_voltage, sensor_data().heater_voltage.load());
    }
    {
        METRIC_DEF(heater_current, "heater_current", METRIC_VALUE_FLOAT, 1005, METRIC_ENABLED);
        metric_record_float(&heater_current, sensor_data().heater_current.load());
    }
    {
        METRIC_DEF(input_current, "input_current", METRIC_VALUE_FLOAT, 1007, METRIC_ENABLED);
        metric_record_float(&input_current, sensor_data().input_current.load());
    }
        #if HAS_MMU2()
    {
        METRIC_DEF(metric_mmu_i, "cur_mmu_imp", METRIC_VALUE_FLOAT, 1008, METRIC_ENABLED);
        metric_record_float(&metric_mmu_i, sensor_data().mmuCurrent.load());
    }
        #endif
    METRIC_DEF(metric_oc_nozzle_fault, "oc_nozz", METRIC_VALUE_INTEGER, 1010, METRIC_ENABLED);
    metric_record_integer(&metric_oc_nozzle_fault, advancedpower.HeaterOvercurentFaultDetected());
    METRIC_DEF(metric_oc_input_fault, "oc_inp", METRIC_VALUE_INTEGER, 1011, METRIC_ENABLED);
    metric_record_integer(&metric_oc_input_fault, advancedpower.OvercurrentFaultDetected());
}
    #elif BOARD_IS_XLBUDDY()
void RecordPowerStats() {
    METRIC_DEF(metric_splitter_5V_current, "splitter_5V_current", METRIC_VALUE_FLOAT, 1000, METRIC_ENABLED);
    metric_record_float(&metric_splitter_5V_current, advancedpower.GetDwarfSplitter5VCurrent());

    {
        METRIC_DEF(metric_24VVoltage, "24VVoltage", METRIC_VALUE_FLOAT, 1001, METRIC_ENABLED);
        metric_record_float(&metric_24VVoltage, sensor_data().inputVoltage.load());
    }
    {
        METRIC_DEF(metric_5VVoltage, "5VVoltage", METRIC_VALUE_FLOAT, 1002, METRIC_ENABLED);
        metric_record_float(&metric_5VVoltage, sensor_data().sandwich5VVoltage.load());
    }
    {
        METRIC_DEF(metric_Sandwitch5VCurrent, "Sandwitch5VCurrent", METRIC_VALUE_FLOAT, 1003, METRIC_ENABLED);
        metric_record_float(&metric_Sandwitch5VCurrent, sensor_data().sandwich5VCurrent.load());
    }
    {
        METRIC_DEF(metric_xlbuddy5VCurrent, "xlbuddy5VCurrent", METRIC_VALUE_FLOAT, 1004, METRIC_ENABLED);
        metric_record_float(&metric_xlbuddy5VCurrent, sensor_data().buddy5VCurrent.load());
    }
}
    #else
        #error "This board doesn't support ADVANCED_POWER"
    #endif

#endif // HAS_ADVANCED_POWER()

void RecordPrintFilename() {
    METRIC_DEF(file_name, "print_filename", METRIC_VALUE_STRING, 5000, METRIC_ENABLED);
    if (marlin_vars().print_state != marlin_server::State::Idle) {
        // The docstring for media_print_filename() advises against using this function; however, there is currently no replacement for it.
        metric_record_string(&file_name, "%s", marlin_vars().media_LFN.get_ptr());
    } else {
        metric_record_string(&file_name, "");
    }
}

#if BOARD_IS_XLBUDDY()
void record_dwarf_internal_temperatures() {
    {
        METRIC_DEF(metric_dwarfBoardTemperature, "dwarf_board_temp", METRIC_VALUE_INTEGER, 1001, METRIC_ENABLED);
        metric_record_integer(&metric_dwarfBoardTemperature, static_cast<int>(SensorData::head_pcb_temperature()));
    }
    {
        METRIC_DEF(metric_dwarfMCUTemperature, "dwarf_mcu_temp", METRIC_VALUE_INTEGER, 1001, METRIC_DISABLED);
        metric_record_integer(&metric_dwarfMCUTemperature, static_cast<int>(SensorData::head_mcu_temperature()));
    }

    // All MCU temperatures
    METRIC_DEF(mcu, "dwarfs_mcu_temp", METRIC_VALUE_CUSTOM, 0, METRIC_DISABLED); // float value, tag "n": extruder index, tag "a": is active extruder
    static auto mcu_should_record = RateLimiter<uint32_t>(1002);
    if (mcu_should_record.check(ticks_ms())) {
        for (auto tool : PhysicalToolIndex::all().skip_all_disabled()) {
            metric_record_custom(&mcu, ",n=%i,a=%i value=%i", tool.to_raw(), stdext::holds_value(PhysicalToolIndex::currently_selected(), tool), static_cast<int>(buddy::puppies::dwarfs[tool].get_mcu_temperature()));
        }
    }

    // All board temperatures
    METRIC_DEF(board, "dwarfs_board_temp", METRIC_VALUE_CUSTOM, 0, METRIC_DISABLED); // float value, tag "n": extruder index, tag "a": is active extruder
    static auto board_should_record = RateLimiter<uint32_t>(1003);
    if (board_should_record.check(ticks_ms())) {
        for (auto tool : PhysicalToolIndex::all().skip_all_disabled()) {
            metric_record_custom(&board, ",n=%i,a=%i value=%i", tool.to_raw(), stdext::holds_value(PhysicalToolIndex::currently_selected(), tool), static_cast<int>(buddy::puppies::dwarfs[tool].get_board_temperature()));
        }
    }
}
#endif

#if HAS_PHASE_STEPPING()
METRIC_DEF(ps_stalled, "ps_stall", METRIC_VALUE_CUSTOM, 0, METRIC_DISABLED);
METRIC_DEF(ps_speed_change, "ps_spd_chng", METRIC_VALUE_CUSTOM, 0, METRIC_DISABLED);

void record_ps_debug_events() {
    using namespace phase_stepping;
    size_t send = 0;
    while (!debug_events_queue.isEmpty()) {
        const auto event = debug_events_queue.dequeue();
        if (marlin_server::is_printing()) {
            // Only send the metrics when we are printing - we don't care for stalls when we don't move
            std::visit(Overloaded {
                           [](const SuddenSpeedChange &e) {
                               metric_record_custom_at_time(&ps_speed_change, e.timestamp, " ax=\"%c\",from=%.1f,to=%.1f",
                                   e.axis,
                                   static_cast<double>(e.original_speed),
                                   static_cast<double>(e.new_speed));
                           },
                           [](const Stalled &e) {
                               metric_record_custom_at_time(&ps_stalled, e.timestamp, " ax=\"%c\",its=%lu",
                                   e.axis,
                                   e.number_of_iteration);
                           } },
                event);
            ++send;
        }
    }
    if (send > 0) {
        log_info(Metrics, "Sent %zu phstep debug events", send);
    }
}
#endif

} // namespace

void buddy::metrics::record() {
    if (!are_metrics_enabled()) {
        return;
    }

#if HAS_PHASE_STEPPING()
    record_ps_debug_events();
#endif
    RecordMarlinVariables();
    RecordRuntimeStats();
    RecordPrintFilename();
#if HAS_ADVANCED_POWER()
    RecordPowerStats();
#endif
#if BOARD_IS_XLBUDDY()
    record_dwarf_internal_temperatures();
#endif
}
