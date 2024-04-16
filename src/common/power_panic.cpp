#include "power_panic.hpp"
#include "timing_precise.hpp"

#include <type_traits>
#include <assert.h>

#include <option/has_puppies.h>
#include <option/has_embedded_esp32.h>
#include <option/has_toolchanger.h>

#if HAS_TOOLCHANGER()
    #include <module/prusa/toolchanger.h>
#endif /*HAS_TOOLCHANGER()*/

#include "marlin_server.hpp"
#include "media.hpp"

#include "../lib/Marlin/Marlin/src/feature/prusa/crash_recovery.hpp"
#include "../lib/Marlin/Marlin/src/module/endstops.h"
#include "../lib/Marlin/Marlin/src/module/temperature.h"
#include "../lib/Marlin/Marlin/src/module/stepper.h"
#include "../lib/Marlin/Marlin/src/module/stepper/trinamic.h"
#include "../lib/Marlin/Marlin/src/gcode/gcode.h"

#include "../lib/Marlin/Marlin/src/feature/print_area.h"
#include "../lib/Marlin/Marlin/src/feature/bedlevel/bedlevel.h"
#if ENABLED(AUTO_BED_LEVELING_UBL)
    #include "../lib/Marlin/Marlin/src/feature/bedlevel/ubl/ubl.h"
#else
    #error "powerpanic currently supports only UBL"
#endif

#if ENABLED(CANCEL_OBJECTS)
    #include "../Marlin/src/feature/cancel_object.h"
#endif
#if ENABLED(PRUSA_TOOL_MAPPING)
    #include "module/prusa/tool_mapper.hpp"
#endif
#if ENABLED(PRUSA_SPOOL_JOIN)
    #include "module/prusa/spool_join.hpp"
#endif

#include "../lib/Marlin/Marlin/src/feature/input_shaper/input_shaper_config.hpp"
#include "../lib/Marlin/Marlin/src/feature/pressure_advance/pressure_advance_config.hpp"

#include "log.h"
#include "w25x.h"
#include "sound.hpp"
#include "bsod.h"
#include "sys.h"
#include "timing.h"
#include "odometer.hpp"
#include "marlin_vars.hpp"

// print progress
#include "M73_PE.h"
#include "../lib/Marlin/Marlin/src/module/printcounter.h"

#include "ili9488.hpp"
#include <option/has_leds.h>
#if HAS_LEDS()
    #include <led_animations/animator.hpp>
#endif

#include <option/has_side_leds.h>
#if HAS_SIDE_LEDS()
    #include <leds/side_strip_control.hpp>
#endif /*HAS_SIDE_LEDS()*/
#include <option/has_puppies.h>
#if HAS_PUPPIES()
    #include "puppies/puppy_task.hpp"
#endif
#include "safe_state.h"
#include "wdt.hpp"

#include <usb_host/usbh_async_diskio.hpp>

// External thread handles required for suspension
extern osThreadId defaultTaskHandle;
extern osThreadId displayTaskHandle;

namespace power_panic {

LOG_COMPONENT_DEF(PowerPanic, LOG_SEVERITY_INFO);

osThreadId ac_fault_task;

void ac_fault_task_main([[maybe_unused]] void const *argument) {

    // suspend until resumed by the fault isr
    vTaskSuspend(NULL);

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
    for (;;) {
        osSignalSet(defaultTaskHandle, ~0UL);
        xTaskAbortDelay(defaultTaskHandle);
    }
}

static constexpr uint32_t FLASH_OFFSET = w25x_pp_start_address;
static constexpr uint32_t FLASH_SIZE = w25x_pp_size;

// WARNING: mind the alignment, the following structures are not automatically packed
// as they're shared also for the on-memory copy. The on-memory copy can be avoided
// if we decide to use two flash blocks (keeping one as a working set)
#pragma GCC diagnostic push
#pragma GCC diagnostic warning "-Wpadded"

// planner state (TODO: a _lot_ of essential state is missing here and Crash_s also due to
// the partial Motion_Parameters implementation)
struct flash_planner_t {
    user_planner_settings_t settings;

    float z_position;
#if DISABLED(CLASSIC_JERK)
    float junction_deviation_mm;
#endif

    int16_t target_nozzle[HOTENDS];
    int16_t flow_percentage[HOTENDS];
    int16_t target_bed;
    int16_t extrude_min_temp;
#if ENABLED(MODULAR_HEATBED)
    uint16_t enabled_bedlets_mask;
    uint8_t _padding_heat[2]; // padding to 2 or 4 bytes?
#endif

    uint16_t print_speed;
    uint8_t was_paused;
    uint8_t was_crashed;
    uint8_t fan_speed;
    uint8_t axis_relative;
    uint8_t allow_cold_extrude;

    uint8_t _padding_is[1];

    // IS/PA
    input_shaper::AxisConfig axis_config[3]; // XYZ
    input_shaper::AxisConfig original_y;
    input_shaper::WeightAdjustConfig axis_y_weight_adjust;
    pressure_advance::Config axis_e_config;
};

// fully independent state that persist across panics until the end of the print
struct flash_print_t {
    float odometer_e_start; /// E odometer value at the start of the print
};

// crash recovery data
struct flash_crash_t {
    uint32_t sdpos; /// sdpos of the gcode instruction being aborted
    xyze_pos_t start_current_position; /// absolute logical starting XYZE position of the gcode instruction
    xyze_pos_t crash_current_position; /// absolute logical XYZE position of the crash location
    abce_pos_t crash_position; /// absolute physical ABCE position of the crash location
    uint16_t segments_finished = 0;
    uint8_t axis_known_position; /// axis state before crashing
    uint8_t leveling_active; /// state of MBL before crashing
    feedRate_t fr_mm_s; /// current move feedrate
    Crash_s_Counters::Data counters;
    Crash_s::InhibitFlags inhibit_flags; /// inhibit instruction replay flags

    uint8_t _padding[1]; // silence warning
};

// print progress data
struct flash_progress_t {
    struct ModeSpecificData {
        uint32_t percent_done;
        uint32_t time_to_end;
        uint32_t time_to_pause;
    };

    millis_t print_duration;
    ModeSpecificData standard_mode, stealth_mode;
};

// toolchanger recovery info
//   can't use PrusaToolChanger::PrecrashData as it doesn't have to be packed
struct flash_toolchanger_t {
#if HAS_TOOLCHANGER()
    xyz_pos_t return_pos; ///< Position wanted after toolchange
    uint8_t precrash_tool; ///< Tool wanted to be picked before panic
    tool_return_t return_type : 8; ///< Where to return after recovery
    uint32_t : 16; ///< Padding to keep the structure size aligned to 32 bit
#endif /*HAS_TOOLCHANGER()*/
};

#pragma GCC diagnostic pop

// Data storage layout
struct flash_data {
    // non-changing print parameters
    struct fixed_t {
        xy_pos_t bounding_rect_a;
        xy_pos_t bounding_rect_b;
        bed_mesh_t z_values;
        char media_SFN_path[FILE_PATH_MAX_LEN];

        static void load();
        static void save();
    } fixed;

    // varying parameters
    struct state_t {
        flash_crash_t crash;
        flash_planner_t planner;
        flash_progress_t progress;
        flash_print_t print;
        flash_toolchanger_t toolchanger;

#if ENABLED(CANCEL_OBJECTS)
        uint32_t canceled_objects;
#endif
#if ENABLED(PRUSA_TOOL_MAPPING)
        ToolMapper::serialized_state_t tool_mapping;
#endif
#if ENABLED(PRUSA_SPOOL_JOIN)
        SpoolJoin::serialized_state_t spool_join;
#endif
        GCodeReaderStreamRestoreInfo gcode_stream_restore_info;
        uint8_t invalid; // set to zero before writing, cleared on erase

        static void load();
        static void save();
    } state;

    static void erase();
    static void sync();
};

static_assert(sizeof(flash_data) <= FLASH_SIZE, "powerpanic data exceeds reserved storage space");

enum class PPState : uint8_t {
    // note: order is important, there is check that PPState >= Triggered
    Inactive,
    Prepared,
    Triggered,
    Retracting,
    SaveState,
    WaitingToDie,
};

std::atomic_bool ac_fault_triggered = false;
static PPState power_panic_state = PPState::Inactive;

// Temporary buffer for state filled at the time of the acFault trigger
static struct {
    bool nested_fault;
    PPState orig_state;
    char media_SFN_path[FILE_PATH_MAX_LEN]; // temporary buffer
    uint8_t orig_axis_known_position;
    uint32_t fault_stamp; // time since acFault trigger

    // Temporary copy to handle nested fault handling
    flash_crash_t crash;
    flash_planner_t planner;
    flash_progress_t progress;
    flash_print_t print;
    flash_toolchanger_t toolchanger;

#if ENABLED(CANCEL_OBJECTS)
    uint32_t canceled_objects;
#endif
#if ENABLED(PRUSA_TOOL_MAPPING)
    ToolMapper::serialized_state_t tool_mapping;
#endif
#if ENABLED(PRUSA_SPOOL_JOIN)
    SpoolJoin::serialized_state_t spool_join;
#endif
    GCodeReaderStreamRestoreInfo gcode_stream_restore_info;
} state_buf;

// Helper functions to read/write to the flash area with type checking
#define FLASH_DATA_OFF(member) (FLASH_OFFSET + offsetof(flash_data, member))

#define FLASH_SAVE(member, src)                                                          \
    do {                                                                                 \
        static_assert(std::is_same<decltype(flash_data::member), decltype(src)>::value); \
        w25x_program(FLASH_DATA_OFF(member), (uint8_t *)(&src), sizeof(src));            \
    } while (0)

#define FLASH_SAVE_EXPR(member, expr)  \
    ([&]() {                           \
        decltype(member) buf = (expr); \
        FLASH_SAVE(member, buf);       \
        return buf;                    \
    }())

#define FLASH_LOAD(member, dst)                                                          \
    do {                                                                                 \
        static_assert(std::is_same<decltype(flash_data::member), decltype(dst)>::value); \
        w25x_rd_data(FLASH_DATA_OFF(member), (uint8_t *)(&dst), sizeof(dst));            \
    } while (0)

#define FLASH_LOAD_EXPR(member)           \
    ([]() {                               \
        decltype(flash_data::member) buf; \
        FLASH_LOAD(member, buf);          \
        return buf;                       \
    }())

void flash_data::fixed_t::save() {
    // print area
    PrintArea::rect_t rect = print_area.get_bounding_rect();
    FLASH_SAVE(fixed.bounding_rect_a, rect.a);
    FLASH_SAVE(fixed.bounding_rect_b, rect.b);

    // mbl
    FLASH_SAVE(fixed.z_values, unified_bed_leveling::z_values);

    // optimize the write to avoid writing the entire buffer
    w25x_program(FLASH_DATA_OFF(fixed.media_SFN_path), (uint8_t *)state_buf.media_SFN_path,
        strlen(state_buf.media_SFN_path) + 1);
}

void flash_data::fixed_t::load() {
    // print area
    PrintArea::rect_t rect = {
        FLASH_LOAD_EXPR(fixed.bounding_rect_a),
        FLASH_LOAD_EXPR(fixed.bounding_rect_b)
    };
    print_area.set_bounding_rect(rect);

    // mbl
    FLASH_LOAD(fixed.z_values, unified_bed_leveling::z_values);

    // path
    FLASH_LOAD(fixed.media_SFN_path, state_buf.media_SFN_path);
}

void flash_data::state_t::save() {
    FLASH_SAVE(state.crash, state_buf.crash);
    FLASH_SAVE(state.planner, state_buf.planner);
    FLASH_SAVE(state.progress, state_buf.progress);
    FLASH_SAVE(state.print, state_buf.print);
    FLASH_SAVE(state.toolchanger, state_buf.toolchanger);
#if ENABLED(CANCEL_OBJECTS)
    FLASH_SAVE(state.canceled_objects, state_buf.canceled_objects);
#endif
#if ENABLED(PRUSA_TOOL_MAPPING)
    FLASH_SAVE(state.tool_mapping, state_buf.tool_mapping);
#endif
#if ENABLED(PRUSA_SPOOL_JOIN)
    FLASH_SAVE(state.spool_join, state_buf.spool_join);
#endif
    FLASH_SAVE(state.gcode_stream_restore_info, state_buf.gcode_stream_restore_info);

    FLASH_SAVE_EXPR(state.invalid, false);
    if (w25x_fetch_error()) {
        log_error(PowerPanic, "Failed to save live data.");
    }
}

void flash_data::state_t::load() {
    FLASH_LOAD(state.crash, state_buf.crash);
    FLASH_LOAD(state.planner, state_buf.planner);
    FLASH_LOAD(state.progress, state_buf.progress);
    FLASH_LOAD(state.print, state_buf.print);
    FLASH_LOAD(state.toolchanger, state_buf.toolchanger);
#if ENABLED(CANCEL_OBJECTS)
    FLASH_LOAD(state.canceled_objects, state_buf.canceled_objects);
#endif
#if ENABLED(PRUSA_TOOL_MAPPING)
    FLASH_LOAD(state.tool_mapping, state_buf.tool_mapping);
#endif
#if ENABLED(PRUSA_SPOOL_JOIN)
    FLASH_LOAD(state.spool_join, state_buf.spool_join);
#endif
    FLASH_LOAD(state.gcode_stream_restore_info, state_buf.gcode_stream_restore_info);
    state_buf.nested_fault = true;
}

void flash_data::erase() {
    for (uintptr_t addr = 0; addr < FLASH_SIZE; addr += W25X_BLOCK_SIZE) {
        w25x_sector_erase(FLASH_OFFSET + addr);
    }
}

void flash_data::sync() {
}

bool state_stored() {
    bool retval = !FLASH_LOAD_EXPR(state.invalid);
    if (w25x_fetch_error()) {
        log_error(PowerPanic, "Failed to get stored.");
        return false;
    }
    return retval;
}

// decide whether to current print can auto-recover
static bool auto_recover_check() {
    if (state_buf.print.odometer_e_start >= Odometer_s::instance().get_extruded_all()) {
        // nothing has been extruded on the bed so far, it's safe to auto-resume irregardless of temp
        return true;
    }

// check the bed temperature
#if ENABLED(MODULAR_HEATBED)
    thermalManager.setEnabledBedletMask(state_buf.planner.enabled_bedlets_mask);
#endif
    float current_bed_temp = thermalManager.degBed();
    bool auto_recover;
    if (!state_buf.planner.target_bed || current_bed_temp >= state_buf.planner.target_bed) {
        auto_recover = true;
    } else {
        auto_recover = (state_buf.planner.target_bed - current_bed_temp) < POWER_PANIC_MAX_BED_DIFF;
    }
    return auto_recover;
}

bool setup_auto_recover_check() {
    assert(state_stored()); // caller is responsible for checking

    // load the data
    flash_data::fixed_t::load();
    flash_data::state_t::load();
    if (w25x_fetch_error()) {
        log_error(PowerPanic, "Setup load failed.");
        return false;
    }

    // immediately update print progress
    print_job_timer.resume(state_buf.progress.print_duration);
    print_job_timer.pause();

    const auto mode_specific = [](const flash_progress_t::ModeSpecificData &mbuf, ClProgressData::ModeSpecificData &pdata) {
        pdata.percent_done.mSetValue(mbuf.percent_done, state_buf.progress.print_duration);
        pdata.percent_done.mSetValue(mbuf.time_to_end, state_buf.progress.print_duration);
        pdata.percent_done.mSetValue(mbuf.time_to_pause, state_buf.progress.print_duration);
    };
    mode_specific(state_buf.progress.standard_mode, oProgressData.standard_mode);
    mode_specific(state_buf.progress.stealth_mode, oProgressData.stealth_mode);

    // decide whether to auto-recover
    return auto_recover_check();
}

const char *stored_media_path() {
    assert(state_stored()); // caller is responsible for checking
    FLASH_LOAD(fixed.media_SFN_path, state_buf.media_SFN_path);
    if (w25x_fetch_error()) {
        log_error(PowerPanic, "Failed to get media path.");
    }
    return state_buf.media_SFN_path;
}

bool panic_is_active() {
    // panic loop is active when state is higher then triggered
    return power_panic_state >= PPState::Triggered;
}

void prepare() {
    // do not erase/save unless we have a path we can use to resume later
    if (!state_buf.nested_fault) {
        // update the internal filename on the first fault
        marlin_vars()->media_SFN_path.copy_to(state_buf.media_SFN_path, sizeof(state_buf.media_SFN_path));
    }

    // erase and save the MBL data
    flash_data::erase();
    flash_data::fixed_t::save();
    if (w25x_fetch_error()) {
        log_error(PowerPanic, "Failed to save fixed data.");
        return;
    }

    log_info(PowerPanic, "powerpanic prepared");
    power_panic_state = PPState::Prepared;
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
        flash_data::erase();
        if (w25x_fetch_error()) {
            log_error(PowerPanic, "Failed to reset.");
        }
    }

    // internal state
    power_panic_state = PPState::Inactive;
    resume_state = ResumeState::Setup;
    state_buf.nested_fault = false;
}

/// transition from a nested_fault to a normal fault atomically
static void atomic_finish() {
    HAL_NVIC_DisableIRQ(buddy::hw::acFault.getIRQn());

#if HAS_TOOLCHANGER()
    if (state_buf.crash.crash_position.y > PrusaToolChanger::SAFE_Y_WITH_TOOL // Was in toolchange area
        && prusa_toolchanger.is_toolchanger_enabled()) { // Toolchanger is installed

        // Continue with toolcrash recovery
        marlin_server::powerpanic_finish_toolcrash();
    } else
#endif /* HAS_TOOLCHANGER() */
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

void resume_print(bool auto_recover) {
    assert(state_stored()); // caller is responsible for checking
    assert(marlin_server::printer_idle()); // caller is responsible for checking

    log_info(PowerPanic, "resuming print");
    state_buf.nested_fault = true;
    if (resume_state == ResumeState::Setup && auto_recover) {
        resume_state = ResumeState::Resume;
    }
    marlin_server::powerpanic_resume_loop(state_buf.media_SFN_path, state_buf.crash.sdpos, auto_recover);
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
    case ResumeState::Resume:
        // setup the paused state
        // This applies for PowerPanic from paused AND from printing too
        // because printing after power up starts from pause
        marlin_server::resume_state_t resume;
        resume.pos = state_buf.crash.crash_current_position;
        resume.fan_speed = state_buf.planner.fan_speed;
        resume.print_speed = state_buf.planner.print_speed;
        HOTEND_LOOP() {
            resume.nozzle_temp[e] = state_buf.planner.target_nozzle[e];
            if (state_buf.planner.was_paused) {
                marlin_server::set_temp_to_display(state_buf.planner.target_nozzle[e], e);
            }
        }
        if (state_buf.planner.was_paused) {
            resume.nozzle_temp_paused = true; // Nozzle temperatures are stored in resume
        }
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
        // planner settings
        planner.apply_settings(state_buf.planner.settings);
        planner.refresh_acceleration_rates();
#if !HAS_CLASSIC_JERK
        planner.max_e_jerk = state_buf.planner.max_jerk.e;
        planner.junction_deviation_mm = state_buf.planner.junction_deviation_mm;
#endif

        // initial planner state (order is relevant!)
        assert(!planner.leveling_active);
        current_position[Z_AXIS] = state_buf.planner.z_position;
        planner.set_position_mm(current_position);
        if (TEST(state_buf.crash.axis_known_position, Z_AXIS)) {
            SBI(axis_known_position, Z_AXIS);
            SBI(axis_homed, Z_AXIS);
        }

        // canceled objects
#if ENABLED(CANCEL_OBJECTS)
        cancelable.canceled = state_buf.canceled_objects;
#endif
#if ENABLED(PRUSA_TOOL_MAPPING)
        tool_mapper.deserialize(state_buf.tool_mapping);
#endif
#if ENABLED(PRUSA_SPOOL_JOIN)
        spool_join.deserialize(state_buf.spool_join);
#endif
        media_set_restore_info(state_buf.gcode_stream_restore_info);

#if HAS_TOOLCHANGER()
        if (state_buf.crash.crash_position.y > PrusaToolChanger::SAFE_Y_WITH_TOOL) { // Was in toolchange area
            prusa_toolchanger.set_precrash_state({ state_buf.toolchanger.precrash_tool,
                state_buf.toolchanger.return_type,
                state_buf.toolchanger.return_pos }); // Set result for tool recovery
            resume_state = ResumeState::Finish; // Do not reheat, do not unpark
            break; // Skip lift and rehome
            // Will continue with toolcrash recovery
        }
#endif /*HAS_TOOLCHANGER()*/

        // lift and rehome
        if (TEST(state_buf.crash.axis_known_position, X_AXIS) || TEST(state_buf.crash.axis_known_position, Y_AXIS)) {
            float z_dist = current_position[Z_AXIS] - state_buf.crash.crash_current_position[Z_AXIS];
            float z_lift = z_dist < Z_HOMING_HEIGHT ? Z_HOMING_HEIGHT - z_dist : 0;
            char cmd_buf[24];
            snprintf(cmd_buf, sizeof(cmd_buf), "G28 X Y D R%f", (double)z_lift);
            marlin_server::enqueue_gcode(cmd_buf);
        }

        if (state_buf.planner.was_paused) {
            resume_state = ResumeState::ParkForPause;
        } else {
            // Set temperature for all nozzles at once
            HOTEND_LOOP() {
                thermalManager.setTargetHotend(state_buf.planner.target_nozzle[e], e);
            }

            resume_state = ResumeState::WaitForHeaters;
        }
        break;

    case ResumeState::WaitForHeaters: {
        // enqueue a proper wait-for-temperature loop
        char cmd_buf[16];
        HOTEND_LOOP() {
            if (state_buf.planner.target_nozzle[e]) {
                snprintf(cmd_buf, sizeof(cmd_buf), "M109 S%d T%d", state_buf.planner.target_nozzle[e], e);
                marlin_server::enqueue_gcode(cmd_buf);
            }
        }
        if (state_buf.planner.target_bed) {
            snprintf(cmd_buf, sizeof(cmd_buf), "M190 S%d", state_buf.planner.target_bed);
            marlin_server::enqueue_gcode(cmd_buf);
        }

        resume_state = ResumeState::Unpark;
        break;
    }

    case ResumeState::Unpark:
        if (queue.has_commands_queued() || planner.processing()) {
            break;
        }

        // forget the XYZ resume position if requested
        if (state_buf.crash.inhibit_flags & Crash_s::INHIBIT_XYZ_REPOSITIONING) {
            LOOP_XYZ(i) {
                state_buf.crash.crash_current_position[i] = current_position[i];
            }
        }

        // unpark only if the position was known
        if (TEST(state_buf.crash.axis_known_position, X_AXIS) && TEST(state_buf.crash.axis_known_position, Y_AXIS)) {
            plan_park_move_to_xyz(state_buf.crash.crash_current_position, NOZZLE_PARK_XY_FEEDRATE, NOZZLE_PARK_Z_FEEDRATE);
        }

        // always unretract
        plan_move_by(PAUSE_PARK_RETRACT_FEEDRATE, 0, 0, 0, PAUSE_PARK_RETRACT_LENGTH / planner.e_factor[active_extruder]);

        resume_state = ResumeState::Finish;
        break;

    case ResumeState::ParkForPause:
        if (queue.has_commands_queued() || planner.processing()) {
            break;
        }

        // return to the parking position
        plan_park_move_to_xyz(state_buf.crash.start_current_position,
            NOZZLE_PARK_XY_FEEDRATE, NOZZLE_PARK_Z_FEEDRATE);
        resume_state = ResumeState::Finish;
        break;

    case ResumeState::Finish:
        if (queue.has_commands_queued() || planner.processing()) {
            break;
        }

        // original planner state
        HOTEND_LOOP() {
            planner.flow_percentage[e] = state_buf.planner.flow_percentage[e];
        }
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
            crash_s.crash_current_position = d.crash_current_position;
            crash_s.crash_position = d.crash_position;
            crash_s.segments_finished = d.segments_finished;
            crash_s.leveling_active = d.leveling_active;
            crash_s.inhibit_flags = d.inhibit_flags;
            crash_s.fr_mm_s = d.fr_mm_s;
            crash_s.counters.restore_data(d.counters);
        }

        atomic_finish();
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

bool shutdown_loop() {
    // shut off devices one-at-a-time in order of power-draw/time saved
    switch (shutdown_state) {
    case 0:
#if HAS_SIDE_LEDS() || HAS_LEDS()
        leds::enter_power_panic();
        break;
#else
        ++shutdown_state;
        [[fallthrough]];
#endif /*HAS_SIDE_LEDS()*/

    case 1:
        ili9488_power_down();
        break;
    case 2:
#if BOARD_IS_XLBUDDY
        hwio_low_power_state();
        break;
#else
        ++shutdown_state;
        [[fallthrough]];
#endif
    default:
        // no more devices to shutdown, do not increment the sequence
        return false;
    }

    // advance the shutdown sequence
    ++shutdown_state;
    return true;
}

bool shutdown_loop_checked() {
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
            static_cast<unsigned>(power_panic_state), static_cast<unsigned>(shutdown_state - 1));
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

#if !HAS_PUPPIES() // Extruders are on puppy boards and dwarf MCUs are reset in powerpanic
        stepperE0.rms_current(POWER_PANIC_E_CURRENT, 1);
#endif /*HAS_PUPPIES()*/

        // extend XY endstops so that we can still retract/park within an interrupted homing move
        soft_endstop.min.x = X_MIN_POS - (X_MAX_POS - X_MIN_POS);
        soft_endstop.max.x = X_MAX_POS + (X_MAX_POS - X_MIN_POS);
        soft_endstop.min.y = Y_MIN_POS - (Y_MAX_POS - Y_MIN_POS);
        soft_endstop.max.y = Y_MAX_POS + (Y_MAX_POS - Y_MIN_POS);

        // resume motion and keep consistent speeds/rates
        crash_s.set_state(Crash_s::RECOVERY);
        planner.refresh_acceleration_rates();

        if (!state_buf.nested_fault && !state_buf.planner.was_paused && !state_buf.planner.was_crashed && all_axes_homed()) {
#if !HAS_PUPPIES()
            // retract if we were printing
            plan_move_by(PAUSE_PARK_RETRACT_FEEDRATE, 0, 0, 0, -PAUSE_PARK_RETRACT_LENGTH / planner.e_factor[active_extruder]);
            stepper.start_moving();
#endif /*HAS_PUPPIES()*/

            // start powering off complex devices
            shutdown_loop_checked();
        }

        log_info(PowerPanic, "powerpanic triggered");
        power_panic_state = PPState::Retracting;
        break;

    case PPState::Retracting:
        if (shutdown_loop_checked()) {
            break;
        }

#if !HAS_PUPPIES()
        disable_e_steppers();
#endif /*HAS_PUPPIES()*/

        // align the Z axis by lifting as little as sensibly possible
        if (TEST(state_buf.orig_axis_known_position, Z_AXIS) && TEST(state_buf.crash.axis_known_position, Z_AXIS)) {
            if (!state_buf.nested_fault || current_position[Z_AXIS] != state_buf.planner.z_position) {
                log_debug(PowerPanic, "Z MSCNT start: %d", stepperZ.MSCNT());

                // lift just 1 cycle if already far enough from the print
                float z_dist = current_position[Z_AXIS] - state_buf.crash.crash_current_position[Z_AXIS];
                bool already_lifted = z_dist >= planner.mm_per_qsteps(Z_AXIS, POWER_PANIC_Z_LIFT_CYCLES);
                uint8_t cycles = (already_lifted ? 1 : POWER_PANIC_Z_LIFT_CYCLES);
                float z_shift = distance_to_reset_point(Z_AXIS, cycles);
                plan_move_by(POWER_PANIC_Z_FEEDRATE, 0, 0, z_shift);
                stepper.start_moving();

                // continue powering off devices
                shutdown_loop_checked();
            }
        }

        power_panic_state = PPState::SaveState;
        break;

    case PPState::SaveState: {
        if (shutdown_loop_checked()) {
            break;
        }

        // Z axis is now aligned
        stepperZ.rms_current(POWER_PANIC_Z_CURRENT, 1);
        log_debug(PowerPanic, "Z MSCNT end: %d", stepperZ.MSCNT());
        state_buf.planner.z_position = current_position[Z_AXIS];

        // timer & progress state
        state_buf.progress.print_duration = print_job_timer.duration();

        const auto mode_specific = [](flash_progress_t::ModeSpecificData &mbuf, const ClProgressData::ModeSpecificData &pdata) {
            mbuf.percent_done = pdata.percent_done.mGetValue();
            mbuf.time_to_end = pdata.time_to_end.mGetValue();
            mbuf.time_to_pause = pdata.time_to_pause.mGetValue();
        };
        mode_specific(state_buf.progress.standard_mode, oProgressData.standard_mode);
        mode_specific(state_buf.progress.stealth_mode, oProgressData.stealth_mode);

#if ENABLED(CANCEL_OBJECTS)
        state_buf.canceled_objects = cancelable.canceled;
#endif
#if ENABLED(PRUSA_TOOL_MAPPING)
        tool_mapper.serialize(state_buf.tool_mapping);
#endif
#if ENABLED(PRUSA_SPOOL_JOIN)
        spool_join.serialize(state_buf.spool_join);
#endif
        state_buf.gcode_stream_restore_info = media_get_restore_info();
#if HAS_TOOLCHANGER()
        // Store tool that was last requested and where to return in case toolchange is ongoing
        state_buf.toolchanger.precrash_tool = prusa_toolchanger.get_precrash().tool_nr;
        state_buf.toolchanger.return_type = prusa_toolchanger.get_precrash().return_type;
        state_buf.toolchanger.return_pos = prusa_toolchanger.get_precrash().return_pos;
#endif /*HAS_TOOLCHANGER()*/

        log_info(PowerPanic, "powerpanic saving");
        if (state_buf.orig_state != PPState::Prepared) {
            // no prepare was performed for this print yet, prepare the flash now
            flash_data::erase();
            flash_data::fixed_t::save();
        }
        flash_data::state_t::save();
        flash_data::sync();

        // commit odometer trip values
        Odometer_s::instance().force_to_eeprom();

        /// Bitmask of axes that are needed to move
        static constexpr uint8_t test_axes = ENABLED(COREXY) ? (_BV(X_AXIS) | _BV(Y_AXIS)) : _BV(X_AXIS);

        if ((state_buf.crash.axis_known_position & test_axes) == test_axes) {
#if ENABLED(XY_LINKED_ENABLE) && DISABLED(COREXY)
            // XBuddy has XY-EN linked, so the following move will indirectly enable Y.
            // In order to conserve power and keep Y disabled, set the chopper off time via SPI instead.
            stepperY.toff(0);
#endif
#if ENABLED(SKEW_CORRECTION)
            planner.skew_factor.xy = 0; // avoid triggering Y due to XY skew correction
#endif
            feedrate_mm_s = POWER_PANIC_X_FEEDRATE;
            destination = current_position;
            const PrintArea::rect_t print_rect = print_area.get_bounding_rect(); // We need to get out of print area
#if HAS_TOOLCHANGER()
            if (state_buf.crash.crash_position.y > PrusaToolChanger::SAFE_Y_WITH_TOOL) { // Is in the toolchange area
                // Do not move X or Y
            } else
#endif /*HAS_TOOLCHANGER()*/
            {
                if ((state_buf.orig_axis_known_position & test_axes) == test_axes) {
                    // axis position is currently known, move to the closest endpoint
#if ENABLED(COREXY)
                    if (std::min(current_position.x - print_rect.a.x, print_rect.b.x - current_position.x)
                        > std::min(current_position.y - print_rect.a.y, print_rect.b.y - current_position.y)) {
                        // Move to Y edge of printer in direction of nearest Y end of print area
                        destination.y = (current_position.y < (print_rect.a.y + print_rect.b.y) / 2 ? Y_MIN_POS : Y_MAX_PRINT_POS);
                    } else
#endif /*ENABLED(COREXY)*/
                    {
                        // Move to X edge of printer in direction of nearest X end of print area
                        destination.x = (current_position.x < (print_rect.a.x + print_rect.b.x) / 2 ? X_MIN_POS : X_MAX_POS);
                    }
                } else {
                    // we might be anywhere, plan some move towards the endstop
                    destination.x = current_position.x - (X_MAX_POS - X_MIN_POS);
                }
                prepare_move_to_destination();
                stepper.start_moving();
            }
        }

        log_info(PowerPanic, "powerpanic complete");
        Sound_Play(eSOUND_TYPE::CriticalAlert);
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
    // AC-fault during initialization //TODO: IXL Remove if after PP is ready
    if constexpr (PRINTER_IS_PRUSA_XL || PRINTER_IS_PRUSA_MK4 || PRINTER_IS_PRUSA_MK3_5) {
        if (power_panic::is_ac_fault_active()) {
#if !PRINTER_IS_PRUSA_MK3_5 // TODO fix error codes
            fatal_error(ErrCode::ERR_ELECTRO_ACF_AT_INIT);
#endif
        }
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
    if (!state_buf.nested_fault) {
        if ((marlin_server::printer_idle() && !marlin_server::printer_paused())
            || marlin_server::aborting_or_aborted() || marlin_server::print_preview()) {
            state_buf.fault_stamp = ticks_ms();
            power_panic_state = PPState::WaitingToDie;
            // will continue in the main loop
            xTaskResumeFromISR(ac_fault_task);
            return;
        }
    }

    // TODO: can be avoided if running at the same priority as STEP_TIMER_PRIO
    CRITICAL_SECTION_START;

    // ensure the crash handler can't be re-triggered
    HAL_NVIC_DisableIRQ(buddy::hw::xDiag.getIRQn());
    HAL_NVIC_DisableIRQ(buddy::hw::yDiag.getIRQn());

    state_buf.orig_state = power_panic_state;
    state_buf.fault_stamp = ticks_ms();
    power_panic_state = PPState::Triggered;

    // power off devices in order of power draw
    state_buf.orig_axis_known_position = axis_known_position;
    disable_XY();
    buddy::hw::hsUSBEnable.write(buddy::hw::Pin::State::high);
#if HAS_EMBEDDED_ESP32()
    buddy::hw::espPower.reset();
#endif

    // stop motion
    if (!state_buf.nested_fault) {
        marlin_vars()->media_SFN_path.copy_to(state_buf.media_SFN_path, sizeof(state_buf.media_SFN_path));
        state_buf.planner.was_paused = marlin_server::printer_paused();
        state_buf.planner.was_crashed = crash_s.did_trigger();
    }

    if (!state_buf.planner.was_crashed) {
        // fault occurred outside of a crash: trigger one now to update the crash position
        crash_s.set_state(Crash_s::TRIGGERED_AC_FAULT);
        crash_s.crash_axis_known_position = state_buf.orig_axis_known_position;
    }

    if (!state_buf.nested_fault) {
        const marlin_server::resume_state_t &resume = *marlin_server::get_resume_data();

        if (state_buf.planner.was_paused) {
            // crash_current_position *is* current_position while the print is paused,
            // so abuse the slot for the restore position instead
            state_buf.crash.sdpos = media_print_get_pause_position();
            state_buf.crash.crash_current_position = resume.pos;
        } else {
            state_buf.crash.sdpos = crash_s.sdpos;
            state_buf.crash.crash_current_position = crash_s.crash_current_position;
        }

        // save crash parameters
#if HAS_TOOLCHANGER()
        if (crash_s.is_toolchange_event()) {
            // Panic during toolchange, use the intended destination for replay
            state_buf.crash.start_current_position = prusa_toolchanger.get_precrash().return_pos;
            toNative(state_buf.crash.start_current_position); // return_pos is in logical coordinates, needs to be modified in place
        } else
#endif /*HAS_TOOLCHANGER()*/
        {
            state_buf.crash.start_current_position = crash_s.start_current_position;
        }
        state_buf.crash.crash_position = crash_s.crash_position;
        state_buf.crash.segments_finished = crash_s.segments_finished;
        state_buf.crash.axis_known_position = crash_s.crash_axis_known_position;
        state_buf.crash.leveling_active = crash_s.leveling_active;
        state_buf.crash.inhibit_flags = crash_s.inhibit_flags;
        state_buf.crash.fr_mm_s = crash_s.fr_mm_s;

        crash_s.counters.increment(Crash_s::Counter::power_panic);
        state_buf.crash.counters = crash_s.counters.backup_data();

        // save print temperatures
        if (state_buf.planner.was_paused || resume.nozzle_temp_paused) { // Paused print or whenever nozzle is cooled down
            HOTEND_LOOP() {
                state_buf.planner.target_nozzle[e] = resume.nozzle_temp[e];
            }
        } else {
            HOTEND_LOOP() {
                state_buf.planner.target_nozzle[e] = thermalManager.degTargetHotend(e);
            }
        }
        if (state_buf.planner.was_paused) {
            state_buf.planner.fan_speed = resume.fan_speed;
            state_buf.planner.print_speed = resume.print_speed;
        } else {
            state_buf.planner.fan_speed = thermalManager.fan_speed[0];
            state_buf.planner.print_speed = marlin_vars()->print_speed;
        }
        state_buf.planner.target_bed = thermalManager.degTargetBed();
#if ENABLED(MODULAR_HEATBED)
        state_buf.planner.enabled_bedlets_mask = thermalManager.getEnabledBedletMask();
#endif
#if ENABLED(PREVENT_COLD_EXTRUSION)
        state_buf.planner.extrude_min_temp = thermalManager.extrude_min_temp;
        state_buf.planner.allow_cold_extrude = thermalManager.allow_cold_extrude;
#endif

        // remaining planner parameters
        HOTEND_LOOP() {
            state_buf.planner.flow_percentage[e] = planner.flow_percentage[e];
        }
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
#endif /*HAS_TOOLCHANGER()*/

        state_buf.planner.settings = planner.user_settings;

#if !HAS_CLASSIC_JERK
        state_buf.planner.max_jerk.e = planner.max_e_jerk;
        state_buf.planner.junction_deviation_mm = planner.junction_deviation_mm;
#endif
    }

    if (state_buf.planner.was_crashed) {
        // fault occured while handling a crash: original crash location has been saved,
        // it's now safe to overwrite with the current intermediate location for parking
        crash_s.set_state(Crash_s::TRIGGERED_AC_FAULT);
    }

    // heaters are *already* disabled via HW, but stop temperature and fan regulation too
    thermalManager.disable_all_heaters();
    thermalManager.zero_fan_speeds();
#if !HAS_PUPPIES() && HAS_TEMP_HEATBREAK && HAS_TEMP_HEATBREAK_CONTROL
    thermalManager.suspend_heatbreak_fan(2000);
#endif

    // stop & disable endstops
    media_print_quick_stop_powerpanic();
    endstops.enable_globally(false);

    // will continue in the main loop
    xTaskResumeFromISR(ac_fault_task);
    CRITICAL_SECTION_END;
}

bool is_ac_fault_active() {
    return buddy::hw::acFault.read() == buddy::hw::Pin::State::low;
}

} // namespace power_panic
