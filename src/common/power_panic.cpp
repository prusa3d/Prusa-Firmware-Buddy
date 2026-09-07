#include "power_panic.hpp"
#include "power_panic_storage.hpp"
#include "timing_precise.hpp"

#include <type_traits>

#include <option/has_modular_bed.h>
#include <option/has_puppies.h>
#include <option/has_dwarf.h>
#include <option/has_embedded_esp32.h>
#include <option/has_remote_bed.h>
#include <option/has_toolchanger.h>
#include <option/has_indx.h>
#include <option/has_chamber_api.h>
#include <option/has_nozzle_cleaner.h>
#include <option/has_motor_current_profiles.h>
#if HAS_MOTOR_CURRENT_PROFILES()
    #include <feature/motor_current_profile/motor_current_profile.hpp>
#endif
#include <option/has_tool_crash_recovery.h>

#if HAS_TOOLCHANGER()
    #include <module/prusa/toolchanger.h>
#endif

#include "marlin_server.hpp"

#include "../lib/Marlin/Marlin/src/feature/prusa/crash_recovery.hpp"
#include "../lib/Marlin/Marlin/src/module/endstops.h"
#include "../lib/Marlin/Marlin/src/module/temperature.h"
#include "../lib/Marlin/Marlin/src/module/stepper.h"
#include "../lib/Marlin/Marlin/src/feature/motordriver_util.h"
#include "../lib/Marlin/Marlin/src/gcode/gcode.h"

#include "../lib/Marlin/Marlin/src/feature/print_area.h"
#include "../lib/Marlin/Marlin/src/feature/bedlevel/bedlevel.h"
#if ENABLED(AUTO_BED_LEVELING_UBL)
    #include "../lib/Marlin/Marlin/src/feature/bedlevel/ubl/ubl.h"
#else
    #error "powerpanic currently supports only UBL"
#endif

#include <option/has_cancel_object.h>
#if HAS_CANCEL_OBJECT()
    #include <feature/cancel_object/cancel_object.hpp>
#endif

#if HAS_TOOL_MAPPING()
    #include "module/prusa/tool_mapper.hpp"
#endif

#include <option/has_spool_join.h>
#if HAS_SPOOL_JOIN()
    #include <module/prusa/spool_join.hpp>
#endif

#if HAS_CHAMBER_API()
    #include <feature/chamber/chamber.hpp>
#endif
#if HAS_NOZZLE_CLEANER()
    #include <nozzle_cleaner.hpp>
#endif
#if HAS_REMOTE_BED()
    #include <feature/remote_bed/remote_bed.hpp>
#endif

#include "../lib/Marlin/Marlin/src/feature/input_shaper/input_shaper_config.hpp"
#include "../lib/Marlin/Marlin/src/feature/pressure_advance/pressure_advance_config.hpp"

#include <logging/log.hpp>

#include "sound.hpp"
#include "bsod.h"
#include <common/sys.hpp>
#include "timing.h"
#include "odometer.hpp"
#include "marlin_vars.hpp"
#include <mapi/motion.hpp>
#include <mapi/feedrates/standard_feedrates.hpp>

// print progress
#include "M73_PE.h"
#include "../lib/Marlin/Marlin/src/module/printcounter.h"

#include <option/has_gui.h>
#if HAS_GUI()
    #include "ili9488.hpp"
#endif

#include <option/has_leds.h>
#if HAS_LEDS()
    #include <leds/led_manager.hpp>
#endif

#if HAS_PUPPIES()
    #include "puppies/puppy_task.hpp"
#endif
#include "safe_state.h"
#include "wdt.hpp"

#include <usb_host/usbh_async_diskio.hpp>
#include <gcode/gcode_reader_restore_info.hpp>
#include <feature/safety_timer/safety_timer.hpp>
#include <option/xbuddy_extension_variant.h>

namespace {

constexpr uint16_t chamber_temp_off = 0xffff;

constexpr bool power_panic_retracts =
    // Dwarf gets turned off during the shutdown loop, so no retraction can happen
    !HAS_DWARF();

constexpr bool power_panic_deretracts = power_panic_retracts
    // Skipped where G12 S21 already pressurizes the nozzle (see WaitForHeaters).
    && !(HAS_NOZZLE_CLEANER() && (PRINTER_IS_PRUSA_COREONE() || PRINTER_IS_PRUSA_COREONEL()))
    && true;

} // namespace

// External thread handles required for suspension
extern osThreadId defaultTaskHandle;
extern osThreadId displayTaskHandle;

namespace power_panic {

LOG_COMPONENT_DEF(PowerPanic, logging::Severity::info);

/// @brief Runtime state of power panic
/// @note runtime means that it is not persistent, just used to store/resume
runtime_state_t runtime_state;
osThreadId ac_fault_task;

void ac_fault_task_main() {

    // suspend until resumed by the fault isr
    vTaskSuspend(NULL);

    // Heater's power cut in ISR, the proper way needs to be done in the Marlin
    // task (currently in manage_heater).

    // Manage fans detects power panic and enforces fans off
    thermalManager.manage_fans();

    // stop & disable endstops
    marlin_server::print_quick_stop_powerpanic();
#if HAS_REMOTE_BED()
    remote_bed::safe_state();
#endif

    endstops.enable_globally(false);

    // disable unnecessary threads
    // TODO: tcp_ip, network
    vTaskSuspend(USBH_MSC_WorkerTaskHandle);

    // workaround for dislayTask locking the crc32 device (should be suspended instead!)
    osThreadSetPriority(displayTaskHandle, osPriorityIdle);
#if HAS_PUPPIES()
    // puppies will be suspended in AC fault - they are powered down and would not communicate anyway
    buddy::puppies::suspend_puppy_task();
#endif

    // switch into reaping mode: break out of any delay/signal wait until suspended
    osThreadSetPriority(NULL, osPriorityIdle);

    // BFW-6419 REMOVEME
    // xTaskAbortDelay is interrupting waiting for mutexes
    freertos::Mutex::power_panic_mode_removeme = true;

    // Keep waking the marlin task from any sleeps()
    // When the marlin task enters panic_loop, this task gets suspended
    // and somewhat standard operation is restored
    for (;;) {
        osSignalSet(defaultTaskHandle, ~0UL);
        xTaskAbortDelay(defaultTaskHandle);
    }
}

std::atomic_bool ac_fault_triggered = false;
std::atomic_bool should_beep = true;
std::atomic<PPState> power_panic_state = PPState::Inactive;

bool panic_is_active() {
    // panic loop is active when state is higher then triggered
    return power_panic_state >= PPState::Triggered;
}

void prepare() {
    // do not erase/save unless we have a path we can use to resume later
    if (!runtime_state.nested_fault) {
        // update the internal filename on the first fault
        marlin_vars().media_SFN_path.copy_to(runtime_state.media_SFN_path, sizeof(runtime_state.media_SFN_path));
    }

    // erase and save the MBL data
    erase();
    fixed_t::save();

    log_info(PowerPanic, "powerpanic prepared");
    power_panic_state = PPState::Prepared;
}

void refresh_sfn() {
    if (power_panic_state != PPState::Prepared) {
        // No MBL ready yet, don't save incomplete data.
        return;
    }
    // Make sure we don't save invalid data if we get PP in the middle of this.
    // Really a rare cornercase, just minimizing the blast radius of such situation.
    power_panic_state = PPState::Inactive;
    prepare();
}

enum class ResumeState : uint8_t {
    Setup,
    Resume,
    WaitForHeaters,
    Unpark,
    ParkForPause,
    Finish,
    Error,
};

std::atomic<ResumeState> resume_state = ResumeState::Setup;

/// reset PP state during atomic_finish (holds print state)
static void atomic_reset() {
    // stored state
    if (state_stored()) {
        erase();
    }

    // internal state
    power_panic_state = PPState::Inactive;
    resume_state = ResumeState::Setup;
    runtime_state.nested_fault = false;
}

/// transition from a nested_fault to a normal fault atomically
static void atomic_finish() {
    HAL_NVIC_DisableIRQ(buddy::hw::acFault.getIRQn());

#if HAS_TOOLCHANGER() && HAS_INDX()
    if (state_buf.toolchanger.phase != PrusaToolChanger::ToolchangePhase::none) {
        // Power panic hit during a toolchange; finish the toolchange on resume
        marlin_server::powerpanic_finish_indx_toolchange();
    } else
#endif
#if HAS_TOOLCHANGER() && HAS_TOOL_CRASH_RECOVERY()
        if (prusa_toolchanger.is_pos_in_toolchange_area(state_buf.crash.crash_machine_position.xy()) && prusa_toolchanger.is_toolchanger_enabled()) {

        // Continue with toolcrash recovery
        marlin_server::powerpanic_finish_toolcrash();
    } else
#endif
    {
        if (state_buf.planner.was_paused) {
            marlin_server::powerpanic_finish_pause();
        } else {
            marlin_server::powerpanic_finish_recovery();
        }
    }
    atomic_reset();

    HAL_NVIC_EnableIRQ(buddy::hw::acFault.getIRQn());
}

void resume_print() {
    debug_assert(state_stored()); // caller is responsible for checking
    debug_assert(marlin_server::printer_idle()); // caller is responsible for checking

    // load the data
    fixed_t::load();
    state_t::load();

    log_info(PowerPanic, "resuming print");
    runtime_state.nested_fault = true;

    // immediately update print progress
    {
        print_job_timer.resume(state_buf.progress.print_duration);
        print_job_timer.pause();

        const auto mode_specific = [](const state_progress_t::ModeSpecificData &mbuf, ClProgressData::ModeSpecificData &pdata) {
            pdata.percent_done.mSetValue(mbuf.percent_done, state_buf.progress.print_duration);
            pdata.time_to_end.mSetValue(mbuf.time_to_end, state_buf.progress.print_duration);
            pdata.time_to_pause.mSetValue(mbuf.time_to_pause, state_buf.progress.print_duration);
        };
        mode_specific(state_buf.progress.standard_mode, oProgressData.standard_mode);
        mode_specific(state_buf.progress.stealth_mode, oProgressData.stealth_mode);
    }

    const bool auto_recover = [] {
        if (state_buf.print.odometer_e_start >= Odometer_s::instance().get_extruded_all()) {
            // nothing has been extruded on the bed so far, it's safe to auto-resume irregardless of temp
            return true;
        }

// check the bed temperature
#if HAS_MODULAR_BED()
        thermalManager.setEnabledBedletMask(state_buf.planner.enabled_bedlets_mask);
#endif
        const float current_bed_temp = thermalManager.degBed();

        if (!state_buf.planner.target_bed || current_bed_temp >= state_buf.planner.target_bed) {
            return true;
        }

        return (state_buf.planner.target_bed - current_bed_temp) < POWER_PANIC_MAX_BED_DIFF;
    }();

    if (resume_state == ResumeState::Setup && auto_recover) {
        resume_state = ResumeState::Resume;
    }

    const GCodeReaderPosition gcode_pos {
        .restore_info = state_buf.gcode_stream_restore_info,
        .offset = state_buf.crash.sdpos,
    };
    marlin_server::powerpanic_resume(runtime_state.media_SFN_path, gcode_pos, auto_recover);
}

void resume_continue() {
    if (resume_state == ResumeState::Setup) {
        resume_state = ResumeState::Resume;
    }
}

void resume_loop() {
    switch (resume_state) {
    case ResumeState::Setup:
        // Set bed temperature to prevent bed from cooling down
        thermalManager.setTargetBed(state_buf.planner.target_bed);
        break;

    case ResumeState::Resume: {
        // setup the paused state
        // This applies for PowerPanic from paused AND from printing too
        // because printing after power up starts from pause
        marlin_server::resume_state_t resume;
        resume.pos = state_buf.crash.crash_native_position;
        resume.fan_speed = state_buf.planner.fan_speed;
        resume.print_speed = state_buf.planner.print_speed;
        resume.nozzle_temp = state_buf.planner.target_nozzle;
#if HAS_MOTOR_CURRENT_PROFILES()
        buddy::set_active_motor_current_profile(static_cast<buddy::StandardMotorCurrentProfile>(state_buf.planner.current_profile));
#endif
#if HAS_INDX()
        resume.active_tool = state_buf.planner.active_tool;
        if (state_buf.toolchanger.phase != PrusaToolChanger::ToolchangePhase::none) {
            // During a toolchange planner.active_tool is pre-toolchange; use the target tool.
            resume.active_tool = state_buf.toolchanger.tool_nr;
        }
#endif
        marlin_server::set_resume_data(&resume);

        // Set sdpos
        //  in case powerpanic happens before sdpos propagates from resume data to media where crash_s would get it
        crash_s.sdpos = state_buf.crash.sdpos;

        // set bed temperatures
        thermalManager.setTargetBed(state_buf.planner.target_bed);
#if ENABLED(PREVENT_COLD_EXTRUSION)
        thermalManager.extrude_min_temp = state_buf.planner.extrude_min_temp;
        thermalManager.allow_cold_extrude = state_buf.planner.allow_cold_extrude;
#endif

        gcode.compatibility = state_buf.planner.compatibility;

        marlin_debug_flags = state_buf.planner.marlin_debug_flags;

        // planner settings
        planner.apply_settings(state_buf.planner.settings);
        planner.refresh_acceleration_rates();
#if !HAS_CLASSIC_JERK
        planner.junction_deviation_mm = state_buf.planner.junction_deviation_mm;
#endif

#if HAS_INDX()
        // active_extruder is not restored from EEPROM (the cache is invalidated
        // mid-print). Apply the saved pre-toolchange tool so subsequent E moves
        // are valid, including its tool offset. Prefer the one physically held
        // if toolchange is in progress.
        const uint8_t effective_active_tool
            = state_buf.toolchanger.phase == PrusaToolChanger::ToolchangePhase::after_lock
            ? state_buf.toolchanger.tool_nr
            : state_buf.planner.active_tool;
        if (effective_active_tool != PrusaToolChanger::MARLIN_NO_TOOL_PICKED) {
            const PhysicalToolIndex tool = PhysicalToolIndex::from_raw(effective_active_tool);
            prusa_toolchanger.set_active_extruder(tool);
            hotend_currently_applied_offset = hotend_offset[tool];
        } else {
            prusa_toolchanger.set_active_extruder(NoTool {});
            hotend_currently_applied_offset = xyz_pos_t {};
        }
#endif
        // initial planner state (order is relevant!)
        debug_assert(!planner.leveling_active);
        current_position[Z_AXIS] = state_buf.planner.z_position;
        planner.set_position_mm(current_position);
        axes_home_level[Z_AXIS] = state_buf.crash.axes_home_level[Z_AXIS];
        planner.max_printed_z = state_buf.planner.max_printed_z;

        // canceled objects
#if HAS_CANCEL_OBJECT()
        buddy::cancel_object().set_state(state_buf.cancel_object);
#endif
#if HAS_TOOL_MAPPING()
        tool_mapper.deserialize(state_buf.tool_mapping);
#endif
#if HAS_SPOOL_JOIN()
        spool_join.deserialize(state_buf.spool_join);
#endif
#if HAS_CHAMBER_API()
        if (state_buf.chamber_target_temp == chamber_temp_off) {
            buddy::chamber().set_target_temperature(std::nullopt);
        } else {
            buddy::chamber().set_target_temperature(state_buf.chamber_target_temp);
        }
#endif
#if HAS_TEMP_HEATBREAK_CONTROL
        for (auto tool : PhysicalToolIndex::all()) {
            thermalManager.setTargetHeatbreak(state_buf.heatbreak_temperatures[tool], tool);
        }

#endif

#if HAS_TOOLCHANGER() && HAS_TOOL_CRASH_RECOVERY()
        if (prusa_toolchanger.is_pos_in_toolchange_area(state_buf.crash.crash_machine_position.xy())) {
            prusa_toolchanger.set_return_data({
                PhysicalToolIndex::from_raw_notool(state_buf.toolchanger.tool_nr),
                state_buf.toolchanger.return_type,
                state_buf.toolchanger.return_pos,
            });
            resume_state = ResumeState::Finish; // Do not reheat, do not unpark
            break; // Skip lift and rehome
            // Will continue with toolcrash recovery
        }
#endif
#if HAS_INDX()
        if (state_buf.toolchanger.phase != PrusaToolChanger::ToolchangePhase::none) {
            // Skip XY re-home. Already included in the toolchange recovery.
            resume_state = ResumeState::Finish;
            break;
        }
#endif

        if (state_buf.crash.recover_flags & Crash_s::RECOVER_AXIS_STATE) {
            // lift and rehome
            if (state_buf.crash.axes_home_level.is_homed(X_AXIS, AxisHomeLevel::imprecise) || state_buf.crash.axes_home_level.is_homed(Y_AXIS, AxisHomeLevel::imprecise)) {
                float z_dist = current_position[Z_AXIS] - state_buf.crash.crash_native_position[Z_AXIS];
                float z_lift = z_dist < Z_HOMING_HEIGHT ? Z_HOMING_HEIGHT - z_dist : 0;
                char cmd_buf[24];
                snprintf(cmd_buf, sizeof(cmd_buf), "G28 X Y D R%f", (double)z_lift);
                marlin_server::enqueue_gcode(cmd_buf);
            }
        }

        if (state_buf.planner.was_paused) {
            resume_state = ResumeState::ParkForPause;
        } else {
            for (auto tool : PhysicalToolIndex::all()) {
                thermalManager.setTargetHotend(state_buf.planner.target_nozzle[tool], tool);
            }
            // setTargetBed is already called higher up in this function

            resume_state = ResumeState::WaitForHeaters;
        }
        break;
    }

    case ResumeState::WaitForHeaters: {
        buddy::safety_timer().reset_restore_nonblocking();

        if (!Temperature::are_all_temperatures_reached()) {
            break;
        }

#if HAS_NOZZLE_CLEANER()
        if (PhysicalToolIndex::currently_selected_opt().has_value()) {
            // Keep the printer list below in sync with the unretract guard in
            // ResumeState::Unpark — G12 S21 pressurizes the nozzle on its own.
    #if PRINTER_IS_PRUSA_COREONE() || PRINTER_IS_PRUSA_COREONEL()
            static_assert(!power_panic_deretracts);
            marlin_server::enqueue_gcode("G12 S90"); // enter cleaner
            marlin_server::enqueue_gcode("G12 S21"); // purge (no retract) and brush wipe
            marlin_server::enqueue_gcode("G12 S91"); // exit cleaner
    #else
            marlin_server::enqueue_gcode("G12"); // clean nozzle on the brush
    #endif
        }
#endif
        resume_state = ResumeState::Unpark;
        break;
    }

    case ResumeState::Unpark:
        if (marlin_server::is_processing()) {
            break;
        }

        // forget the XYZ resume position if requested
        if (!(state_buf.crash.recover_flags & Crash_s::RECOVER_XY_POSITION)) {
            LOOP_XY(i) {
                state_buf.crash.crash_native_position[i] = current_position[i];
            }
        }
        if (!(state_buf.crash.recover_flags & Crash_s::RECOVER_Z_POSITION)) {
            state_buf.crash.crash_native_position[Z_AXIS] = current_position[Z_AXIS];
        }

        // unpark only if the position was known
        if (state_buf.crash.axes_home_level.is_homed({ X_AXIS, Y_AXIS }, AxisHomeLevel::imprecise)) {
            plan_park_move_to_xyz(state_buf.crash.crash_native_position.xyz(), NOZZLE_PARK_XY_FEEDRATE, NOZZLE_PARK_Z_FEEDRATE, Segmented::yes);
        }

        // Unretract paired with the retract in PPState::Prepared.
        if (power_panic_deretracts && PhysicalToolIndex::currently_selected_opt().has_value()) {
            mapi::extruder_move(STANDARD_RETRACT_LENGTH, buddy::standard_feedrates::current_extruder(buddy::standard_feedrates::Extruder::deretract));
        }

        resume_state = ResumeState::Finish;
        break;

    case ResumeState::ParkForPause:
        if (marlin_server::is_processing()) {
            break;
        }

        // return to the parking position
        plan_park_move_to_xyz(state_buf.crash.start_current_position.xyz(),
            NOZZLE_PARK_XY_FEEDRATE, NOZZLE_PARK_Z_FEEDRATE, Segmented::yes);
        resume_state = ResumeState::Finish;
        break;

    case ResumeState::Finish:
        if (marlin_server::is_processing()) {
            break;
        }

        // original planner state
        planner.flow_percentage = state_buf.planner.flow_percentage;
        gcode.axis_relative = state_buf.planner.axis_relative;

        // IS/PA
        LOOP_XYZ(i) {
            if (state_buf.planner.axis_config[i].frequency == 0.f) {
                input_shaper::set_axis_config((AxisEnum)i, std::nullopt);
            } else {
                input_shaper::set_axis_config((AxisEnum)i, state_buf.planner.axis_config[i]);
            }
        }

        if (state_buf.planner.original_y.frequency == 0.f) {
            input_shaper::set_config_for_m74(Y_AXIS, std::nullopt);
        } else {
            input_shaper::set_config_for_m74(Y_AXIS, state_buf.planner.original_y);
        }

        if (state_buf.planner.axis_y_weight_adjust.frequency_delta != 0.f) {
            input_shaper::set_axis_y_weight_adjust(std::nullopt);
        } else {
            input_shaper::set_axis_y_weight_adjust(state_buf.planner.axis_y_weight_adjust);
        }

        pressure_advance::set_axis_e_config(state_buf.planner.axis_e_config);

        // restore crash state
        {
            const auto &d = state_buf.crash;

            crash_s.start_current_position = d.start_current_position;
            crash_s.crash_native_position = d.crash_native_position;
            crash_s.crash_machine_position = d.crash_machine_position;
            crash_s.segments_finished = d.segments_finished;
            crash_s.leveling_active = d.leveling_active;
            crash_s.recover_flags = d.recover_flags;
            crash_s.fr_mm_s = d.fr_mm_s;
            crash_s.counters.restore_data(d.counters);
        }

        atomic_finish();

        // Re-prepare for the next power panic now that power is stable, to
        // avoid expensive erase.
        //
        // For the non-resumed print, it is done in MBL/G29.
        prepare();

        log_info(PowerPanic, "resuming complete");

        resume_state = ResumeState::Error;
        break;

    case ResumeState::Error:
        // fail if marlin_server::powerpanic_finish_xxx didn't reset the server loop state
        bsod("resume loop not reset");
    }
}

bool is_power_panic_resuming() {
    return resume_state > ResumeState::Setup;
}

/// fully reset PP state for a new print
void reset() {
    // reset all internal state
    atomic_reset();

    // also reset print state
    state_buf.print.odometer_e_start = Odometer_s::instance().get_extruded_all();

    log_info(PowerPanic, "powerpanic reset");
}

float distance_to_reset_point(const AxisEnum axis, uint8_t min_cycles) {
    return planner.mm_per_qsteps(axis, min_cycles) + planner.distance_to_stepper_zero(axis, has_inverted_axis(axis));
}

uint8_t shutdown_state = 0;

enum class ShutdownState {
#if BOARD_IS_XBUDDY()
    mmu,
#endif
#if HAS_LEDS()
    leds,
#endif
#if HAS_GUI()
    display,
#endif
#if BOARD_IS_XLBUDDY()
    hwio,
#endif
};

bool shutdown_loop() {
    // shut off devices one-at-a-time in order of power-draw/time saved
    switch (static_cast<ShutdownState>(shutdown_state)) {

#if BOARD_IS_XBUDDY()
    case ShutdownState::mmu:
        // Cut power to the MMU connector
        buddy::hw::ext_pwr_enable.reset();
        break;
#endif

#if HAS_LEDS()
    case ShutdownState::leds:
        leds::LEDManager::instance().enter_power_panic();
        break;
#endif
#if HAS_GUI()
    case ShutdownState::display:
        ili9488_power_down();
        break;
#endif

#if BOARD_IS_XLBUDDY()
    case ShutdownState::hwio:
        hwio_low_power_state();
        break;
#endif

    default:
        // no more devices to shutdown, do not increment the sequence
        return false;
    }

    // advance the shutdown sequence
    ++shutdown_state;
    return true;
}

/// Periodic check whether the planned move has finished.
/// If the planner is running, the procedure starts shutting down devices in parallel
/// (because we're waiting for the moves to finish and have some spare time)
/// @returns false when all moves have finished
bool shutdown_devices_while_moving() {
    bool processing = planner.processing();
    if (!processing) {
        // no time to perform any shutdown
        return false;
    }

    // try to run one iteration of the shutdown sequence
    if (planner.has_unprocessed_blocks_queued() || stepper.segment_progress() < 0.5f) {
        shutdown_loop();
    }

    // check that no single step takes too long, emit a warning so that we can notice and re-arrange
    // the sequence to avoid stalling (@wavexx consider the move time above if just looking at the
    // move above is not sufficient)
    processing = planner.processing();
    if (!processing) {
        log_warning(PowerPanic, "shutdown state %u/%u took too long",
            static_cast<unsigned>(power_panic_state.load()), static_cast<unsigned>(shutdown_state - 1));
    }

    return processing;
}

void panic_loop() {
    switch (power_panic_state) {
    case PPState::Triggered:
        // suspend the helper task
        vTaskSuspend(ac_fault_task);
        log_debug(PowerPanic, "powerpanic loop start");

        // reduce power of motors
        stepperX.rms_current(POWER_PANIC_X_CURRENT, 1);
#if ENABLED(COREXY)
        // XY are linked, set both motors to the same current
        stepperY.rms_current(POWER_PANIC_X_CURRENT, 1);
#endif /*ENABLED(COREXY)*/

#if !HAS_DWARF() // Extruders are on puppy boards and dwarf MCUs are reset in powerpanic
        stepperE0.rms_current(POWER_PANIC_E_CURRENT, 1);
#endif

        // extend XY endstops so that we can still retract/park within an interrupted homing move
        soft_endstop.min.x = X_MIN_POS - (X_MAX_POS - X_MIN_POS);
        soft_endstop.max.x = X_MAX_POS + (X_MAX_POS - X_MIN_POS);
        soft_endstop.min.y = Y_MIN_POS - (Y_MAX_POS - Y_MIN_POS);
        soft_endstop.max.y = Y_MAX_POS + (Y_MAX_POS - Y_MIN_POS);

        // resume motion and keep consistent speeds/rates
        crash_s.set_state(Crash_s::RECOVERY);
        planner.refresh_acceleration_rates();

        if (power_panic_retracts && PhysicalToolIndex::currently_selected_opt().has_value() && !runtime_state.nested_fault && !state_buf.planner.was_paused && !state_buf.planner.was_crashed) {
            // retract if we were printing
            mapi::extruder_move(-STANDARD_RETRACT_LENGTH, buddy::standard_feedrates::current_extruder(buddy::standard_feedrates::Extruder::retract));
            planner.start_moving();
        }

        // If we didn't prepare (why?), do so in parallel to retracting E, to save some time.
        if (runtime_state.orig_state != PPState::Prepared) {
            // Maybe do some shutdown first - it's fast and can save energy
            shutdown_devices_while_moving();

            prepare();
        }

        log_info(PowerPanic, "powerpanic triggered");
        power_panic_state = PPState::Retracting;
        break;

    case PPState::Retracting:
        if (shutdown_devices_while_moving()) {
            // Do not continue until queued moves are finished
            break;
        }

#if !HAS_DWARF()
        disable_e_steppers();
#endif

        // align the Z axis by lifting as little as sensibly possible
        if (runtime_state.orig_axes_home_level.is_homed(Z_AXIS, AxisHomeLevel::imprecise) && state_buf.crash.axes_home_level.is_homed(Z_AXIS, AxisHomeLevel::imprecise)) {
            if (!runtime_state.nested_fault || current_position[Z_AXIS] != state_buf.planner.z_position) {
                log_debug(PowerPanic, "Z MSCNT start: %d", stepperZ.MSCNT());

                // lift just 1 cycle if already far enough from the print
                float z_dist = current_position[Z_AXIS] - state_buf.crash.crash_native_position[Z_AXIS];
                bool already_lifted = z_dist >= planner.mm_per_qsteps(Z_AXIS, POWER_PANIC_Z_LIFT_CYCLES);
                uint8_t cycles = (already_lifted ? 1 : POWER_PANIC_Z_LIFT_CYCLES);
                float z_shift = distance_to_reset_point(Z_AXIS, cycles);
                planner.buffer_line(planner.position_float + MachinePosXYZE { .z = z_shift }, POWER_PANIC_Z_FEEDRATE, PhysicalToolIndex::currently_selected());
                set_current_position(to_native_pos(planner.get_machine_position_mm()));
                planner.start_moving();
            }
        }

        power_panic_state = PPState::SaveState;
        break;

    case PPState::SaveState: {
        if (shutdown_devices_while_moving()) {
            // Do not continue until queued moves are finished
            break;
        }

        // Z axis is now aligned
        stepperZ.rms_current(POWER_PANIC_Z_CURRENT, 1);
        log_debug(PowerPanic, "Z MSCNT end: %d", stepperZ.MSCNT());
        state_buf.planner.z_position = current_position[Z_AXIS];
        state_buf.planner.max_printed_z = planner.max_printed_z;

        // timer & progress state
        state_buf.progress.print_duration = print_job_timer.duration();

        const auto mode_specific = [](state_progress_t::ModeSpecificData &mbuf, const ClProgressData::ModeSpecificData &pdata) {
            mbuf.percent_done = pdata.percent_done.mGetValue();
            mbuf.time_to_end = pdata.time_to_end.mGetValue();
            mbuf.time_to_pause = pdata.time_to_pause.mGetValue();
        };
        mode_specific(state_buf.progress.standard_mode, oProgressData.standard_mode);
        mode_specific(state_buf.progress.stealth_mode, oProgressData.stealth_mode);

#if HAS_CANCEL_OBJECT()
        state_buf.cancel_object = buddy::cancel_object().state();
#endif
#if HAS_TOOL_MAPPING()
        tool_mapper.serialize(state_buf.tool_mapping);
#endif
#if HAS_SPOOL_JOIN()
        spool_join.serialize(state_buf.spool_join);
#endif
#if HAS_CHAMBER_API()
        state_buf.chamber_target_temp = static_cast<uint16_t>(buddy::chamber().target_temperature().value_or(chamber_temp_off));
#endif
#if HAS_TEMP_HEATBREAK_CONTROL
        for (auto tool : PhysicalToolIndex::all()) {
            state_buf.heatbreak_temperatures[tool] = Temperature::degTargetHeatbreak(tool);
        }
#endif
        state_buf.gcode_stream_restore_info = marlin_server::stream_restore_info();
#if HAS_TOOLCHANGER() && (HAS_TOOL_CRASH_RECOVERY() || HAS_INDX())
        // Store tool that was last requested and where to return in case toolchange is ongoing
        {
            const PrusaToolChanger::ToolchangeReturnData &tc = prusa_toolchanger.return_data();
            state_buf.toolchanger.tool_nr = match(
                tc.tool,
                [](PhysicalToolIndex t) -> uint8_t { return t.to_raw(); },
                [](NoTool) -> uint8_t { return PrusaToolChanger::MARLIN_NO_TOOL_PICKED; });
            state_buf.toolchanger.return_type = tc.return_type;
            state_buf.toolchanger.return_pos = tc.return_pos;
        }
#endif

        log_info(PowerPanic, "powerpanic saving");
        state_t::save();

        // commit odometer trip values
        Odometer_s::instance().force_to_eeprom();

        /// Bitmask of axes that are needed to move
        static constexpr std::array test_axes
#if ENABLED(COREXY)
            { X_AXIS, Y_AXIS };
#else
            { X_AXIS };
#endif

        if (state_buf.crash.axes_home_level.is_homed(test_axes, AxisHomeLevel::imprecise)) {
#if ENABLED(XY_LINKED_ENABLE) && DISABLED(COREXY)
            // XBuddy has XY-EN linked, so the following move will indirectly enable Y.
            // In order to conserve power and keep Y disabled, set the chopper off time via SPI instead.
            stepperY.toff(0);
#endif
            destination = current_position;
            const PrintArea::rect_t print_rect = print_area.get_bounding_rect(); // We need to get out of print area
#if HAS_TOOLCHANGER()
            bool stay_put = prusa_toolchanger.is_pos_in_toolchange_area(state_buf.crash.crash_machine_position.xy());
    #if HAS_INDX()
            stay_put |= state_buf.crash.crash_machine_position.x > X_WASTEBIN_SAFE_POINT; // Cleaner / wastebin strip
    #elif PRINTER_IS_PRUSA_XL()
    #else
        #error "Need to know where the toolchanger is"
    #endif
            if (stay_put) { // Outside print area - no need to escape, and lateral move could hit hardware
                // Do not move X or Y
            } else
#endif
            {
                if (runtime_state.orig_axes_home_level.is_homed(test_axes, AxisHomeLevel::imprecise)) {
                    // axis position is currently known, move to the closest endpoint
#if ENABLED(COREXY)
                    if (std::min(current_position.x - print_rect.a.x, print_rect.b.x - current_position.x)
                        > std::min(current_position.y - print_rect.a.y, print_rect.b.y - current_position.y)) {
                        // Move to the print-area Y edge in the direction of the nearest end of the
                        // print area. Both Y_MIN_PRINT_POS and Y_MAX_PRINT_POS exclude any toolchanger
                        // dock zone (per-printer config), so this never crosses dock hardware.
                        current_position.y = (current_position.y < (print_rect.a.y + print_rect.b.y) / 2
                                ? Y_MIN_PRINT_POS
                                : Y_MAX_PRINT_POS);
                    } else
#endif /*ENABLED(COREXY)*/
                    {
                        // Move to X edge of printer in direction of nearest X end of print area.
                        // X_*_PRINT_POS excludes per-printer hardware outside the print area (nozzle cleaner / waste bin).
                        current_position.x = (current_position.x < (print_rect.a.x + print_rect.b.x) / 2 ? X_MIN_PRINT_POS : X_MAX_PRINT_POS);
                    }
                } else {
                    // we might be anywhere, plan some move towards the endstop
                    current_position.x = current_position.x - (X_MAX_POS - X_MIN_POS);
                }
                line_to_current_position(POWER_PANIC_X_FEEDRATE);
                planner.start_moving();
            }
        }

        log_info(PowerPanic, "powerpanic complete");
        if (should_beep) {
            sound::play(SoundType::critical_alert);
        }
        power_panic_state = PPState::WaitingToDie;
        break;
    }

    case PPState::WaitingToDie:
        // turn off any remaining peripherals
        while (shutdown_loop()) {
        }

        // power panic is handled, stop execution of main thread, and wait here until CPU dies
        // Wait time is longer then WDG period, so we'll refresh watchdog few times to avoid dying of dog bites
        // Remember osDelay does not work here as ac_fault_task repeatedly calls xTaskAbortDelay
        for (int _ = 0; _ < POWER_PANIC_HOLD_RST_MS; ++_) {
            wdt_iwdg_refresh();
            delay_us_precise(1000);
        }

        sys_reset();

    case PPState::Inactive:
    case PPState::Prepared:
        // state not reached in this context
        break;
    }
}

std::atomic<bool> ac_fault_enabled = false;

void check_ac_fault_at_startup() {
    if (power_panic::is_ac_fault_active()) {
        fatal_error(ErrCode::ERR_ELECTRO_ACF_AT_INIT);
    }
    ac_fault_enabled = true;
}

void ac_fault_isr() {
    if (!ac_fault_enabled) {
        return;
    }

    // Mark ac_fault as triggered
    ac_fault_triggered = true;

    // prevent re-entry
    HAL_NVIC_DisableIRQ(buddy::hw::acFault.getIRQn());

    // check if handling the fault is worth it (printer is active or can be resumed)
    if (!runtime_state.nested_fault) {
        if ((marlin_server::printer_idle() && !marlin_server::printer_paused())
            || marlin_server::aborting_or_aborted() || marlin_server::print_preview()) {
            runtime_state.fault_stamp = ticks_ms();
            power_panic_state = PPState::WaitingToDie;
            // will continue in the main loop

            // Without the yield, the interrupted task would keep running until the next tick, delaying the ac_fault task.
            const BaseType_t higher_priority_task_woken = xTaskResumeFromISR(ac_fault_task);
            portYIELD_FROM_ISR(higher_priority_task_woken);
            return;
        }
    }

    // TODO: can be avoided if running at the same priority as STEP_TIMER_PRIO
    buddy::InterruptDisabler _;

    // ensure the crash handler can't be re-triggered
    HAL_NVIC_DisableIRQ(buddy::hw::xDiag.getIRQn());
    HAL_NVIC_DisableIRQ(buddy::hw::yDiag.getIRQn());

    runtime_state.orig_state = power_panic_state;
    runtime_state.fault_stamp = ticks_ms();
    power_panic_state = PPState::Triggered;

    // power off devices in order of power draw
    runtime_state.orig_axes_home_level = axes_home_level;
    disable_XY();
    // Cuts power to heaters "the hard way". Proper one is in manage_heater.
    buddy_disable_heaters();
    buddy::hw::hsUSBEnable.write(buddy::hw::Pin::State::high);
#if HAS_EMBEDDED_ESP32()
    buddy::hw::espPower.reset();
#endif

    // stop motion
    if (!runtime_state.nested_fault) {
        state_buf.planner.was_paused = marlin_server::printer_paused();
        state_buf.planner.was_crashed = crash_s.did_trigger();
    }

    if (!state_buf.planner.was_crashed) {
        // fault occurred outside of a crash: trigger one now to update the crash position
        crash_s.set_state(Crash_s::TRIGGERED_AC_FAULT);
        crash_s.crash_axes_home_level = runtime_state.orig_axes_home_level;
    }

    if (!runtime_state.nested_fault) {
        const marlin_server::resume_state_t &resume = *marlin_server::get_resume_data();

        if (state_buf.planner.was_paused) {
            // crash_native_position *is* current_position while the print is paused,
            // so abuse the slot for the restore position instead
            state_buf.crash.sdpos = marlin_server::media_position();
            state_buf.crash.crash_native_position = resume.pos;
        } else {
            state_buf.crash.sdpos = crash_s.sdpos;
            state_buf.crash.crash_native_position = crash_s.crash_native_position;
        }

        // save crash parameters
#if HAS_TOOLCHANGER() && (HAS_TOOL_CRASH_RECOVERY() || HAS_INDX())
    #if HAS_INDX()
        // Snapshot the phase before the preempted tool_change()'s ScopeGuard
        // runs and wipes phase_. The acquire-load also acts as a publication fence for the
        // return_data() read below.
        state_buf.toolchanger.phase = prusa_toolchanger.phase();
    #endif
        if (crash_s.is_toolchange_event()) {
            // Panic during toolchange, use the intended destination for replay
            // !! We're losing E somewhere?!
            state_buf.crash.start_current_position = xyze_pos_t(prusa_toolchanger.return_data().return_pos.asNative());
        } else
#endif
        {
            state_buf.crash.start_current_position = crash_s.start_current_position;
        }
        state_buf.crash.crash_machine_position = crash_s.crash_machine_position;
        state_buf.crash.segments_finished = crash_s.segments_finished;
        state_buf.crash.axes_home_level = crash_s.crash_axes_home_level;
        state_buf.crash.leveling_active = crash_s.leveling_active;
        state_buf.crash.recover_flags = crash_s.recover_flags;
        state_buf.crash.fr_mm_s = crash_s.fr_mm_s;

        crash_s.counters.increment(Crash_s::Counter::power_panic);
        state_buf.crash.counters = crash_s.counters.backup_data();

        // save print temperatures
        if (state_buf.planner.was_paused) { // Paused print or whenever nozzle is cooled down
            state_buf.planner.target_nozzle = resume.nozzle_temp;

        } else {
            state_buf.planner.target_nozzle = buddy::safety_timer().original_hotend_targets();
        }

        if (state_buf.planner.was_paused) {
            state_buf.planner.fan_speed = resume.fan_speed;
            state_buf.planner.print_speed = resume.print_speed;
#if HAS_INDX()
            state_buf.planner.active_tool = resume.active_tool;
#endif
        } else {
            state_buf.planner.fan_speed = thermalManager.print_fan_speed;
            state_buf.planner.print_speed = marlin_vars().print_speed;
#if HAS_INDX()
            state_buf.planner.active_tool = match(
                PhysicalToolIndex::currently_selected(),
                [](PhysicalToolIndex tool) { return tool.to_raw(); },
                [](NoTool) { return PrusaToolChanger::MARLIN_NO_TOOL_PICKED; });
#endif
        }
#if HAS_MOTOR_CURRENT_PROFILES()
        state_buf.planner.current_profile = static_cast<uint8_t>(buddy::active_motor_current_profile());
#endif
        state_buf.planner.target_bed = thermalManager.degTargetBed();
#if HAS_MODULAR_BED()
        state_buf.planner.enabled_bedlets_mask = thermalManager.getEnabledBedletMask();
#endif
#if ENABLED(PREVENT_COLD_EXTRUSION)
        state_buf.planner.extrude_min_temp = thermalManager.extrude_min_temp;
        state_buf.planner.allow_cold_extrude = thermalManager.allow_cold_extrude;
#endif

        // remaining planner parameters
        state_buf.planner.flow_percentage = planner.flow_percentage;
        state_buf.planner.axis_relative = gcode.axis_relative;

        // IS/PA
        LOOP_XYZ(i) {
            if (!input_shaper::current_config().axis[i]) {
                state_buf.planner.axis_config[i].frequency = 0.f;
            } else {
                state_buf.planner.axis_config[i] = *input_shaper::current_config().axis[i];
            }
        }

        if (!input_shaper::get_config_for_m74().axis[Y_AXIS]) {
            state_buf.planner.original_y.frequency = 0.f;
        } else {
            state_buf.planner.original_y = *input_shaper::get_config_for_m74().axis[Y_AXIS];
        }

        if (!input_shaper::current_config().weight_adjust_y) {
            state_buf.planner.axis_y_weight_adjust.frequency_delta = 0.f;
        } else {
            state_buf.planner.axis_y_weight_adjust = *input_shaper::current_config().weight_adjust_y;
        }

        state_buf.planner.axis_e_config = pressure_advance::get_axis_e_config();

#if HAS_TOOLCHANGER()
        // Restore planner config if it was changed by toolchange
        prusa_toolchanger.try_restore();
#endif

        state_buf.planner.settings = planner.user_settings;

#if !HAS_CLASSIC_JERK
        state_buf.planner.junction_deviation_mm = planner.junction_deviation_mm;
#endif
    }

    if (state_buf.planner.was_crashed) {
        // fault occured while handling a crash: original crash location has been saved,
        // it's now safe to overwrite with the current intermediate location for parking
        crash_s.set_state(Crash_s::TRIGGERED_AC_FAULT);
    }

    state_buf.planner.compatibility = gcode.compatibility;

    static_assert(
        std::is_same_v<
            decltype(state_buf.planner.marlin_debug_flags),
            decltype(marlin_debug_flags)>
        == true);
    state_buf.planner.marlin_debug_flags = marlin_debug_flags;

    // will continue in the main loop

    // Without the yield, the interrupted task would keep running until the next tick, delaying the ac_fault task.
    const BaseType_t higher_priority_task_woken = xTaskResumeFromISR(ac_fault_task);
    portYIELD_FROM_ISR(higher_priority_task_woken);
}

bool is_ac_fault_active() {
    return buddy::hw::acFault.read() == buddy::hw::Pin::State::low;
}

} // namespace power_panic
