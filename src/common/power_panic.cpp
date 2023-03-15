#include "power_panic.hpp"

#include <type_traits>
#include <assert.h>

#include <option/has_puppies.h>
#include <option/has_embedded_esp32.h>

#include "marlin_server.hpp"
#include "media.h"

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
#if HAS_LEDS
    #include "../guiapi/include/gui_leds.hpp"
#endif

// External thread handles required for suspension
extern osThreadId defaultTaskHandle;
extern osThreadId displayTaskHandle;

// External support functions in marlin_server
void marlin_server_powerpanic_resume_loop(const char *media_SFN_path, uint32_t pos, bool start_paused);
void marlin_server_powerpanic_finish(bool paused);

namespace power_panic {

LOG_COMPONENT_DEF(PowerPanic, LOG_SEVERITY_INFO);

osThreadId ac_fault_task;

void ac_fault_main(void const *argument) {
    // XL is not able to boot with power panic active (puppies are waiting in bootloader).
    if constexpr (PRINTER_TYPE == PRINTER_PRUSA_XL) {
        if (is_panic_signal()) {
            fatal_error("Power panic is active.", "Power panic");
        }
    }

    // suspend until resumed by the fault isr
    vTaskSuspend(NULL);

    // disable unnecessary threads
    // TODO: tcp_ip, network, USBH_Thread

    // workaround for dislayTask locking the crc32 device (should be suspended instead!)
    osThreadSetPriority(displayTaskHandle, osPriorityIdle);

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
    planner_settings_t settings;
    xyze_pos_t max_jerk;

    float z_position;
    float extruder_advance_K[HOTENDS];
#if DISABLED(CLASSIC_JERK)
    float junction_deviation_mm;
#endif

    int16_t target_nozzle[HOTENDS];
    int16_t flow_percentage[HOTENDS];
    int16_t target_bed;
    int16_t extrude_min_temp;
#if ENABLED(MODULAR_HEATBED)
    uint16_t enabled_bedlets_mask;
#endif

    uint8_t was_paused;
    uint8_t was_crashed;
    uint8_t fan_speed;
    uint8_t axis_relative;
    uint8_t allow_cold_extrude;

    uint8_t _padding[ENABLED(MODULAR_HEATBED) ? 1 : 3];
};

// fully independent state that persist across panics until the end of the print
struct flash_print_t {
    float odometer_e_start; /// E odometer value at the start of the print
};

// crash recovery data
struct flash_crash_t {
    uint32_t sdpos;                    /// sdpos of the gcode instruction being aborted
    xyze_pos_t start_current_position; /// absolute logical starting XYZE position of the gcode instruction
    xyze_pos_t crash_current_position; /// absolute logical XYZE position of the crash location
    abce_pos_t crash_position;         /// absolute physical ABCE position of the crash location
    uint16_t segments_finished = 0;
    uint8_t axis_known_position;         /// axis state before crashing
    uint8_t leveling_active;             /// state of MBL before crashing
    feedRate_t fr_mm_s;                  /// current move feedrate
    xy_uint_t counter_crash = { 0, 0 };  /// number of crashes per axis
    uint16_t counter_power_panic = 0;    /// number of power panics
    Crash_s::InhibitFlags inhibit_flags; /// inhibit instruction replay flags

    uint8_t _padding[1]; // silence warning
};

// print progress data
struct flash_progress_t {
    millis_t print_duration;
    uint32_t percent_done;
    uint32_t time_to_end;
    uint32_t time_to_pause;
};
#pragma GCC diagnostic pop

// Data storage layout
struct __attribute__((packed)) flash_data {
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

#if ENABLED(CANCEL_OBJECTS)
        uint32_t canceled_objects;
#endif

        uint8_t invalid; // set to zero before writing, cleared on erase

        static void load();
        static void save();
    } state;

    static void erase();
    static void sync();
};

static_assert(sizeof(flash_data) <= FLASH_SIZE, "powerpanic data exceeds reserved storage space");

// Temporary buffer for state filled at the time of the acFault trigger
static struct {
    bool nested_fault;
    AcFaultState orig_state;
    char media_SFN_path[FILE_PATH_MAX_LEN]; // temporary buffer
    uint8_t orig_axis_known_position;
    uint32_t fault_stamp; // time since acFault trigger

    // Temporary copy to handle nested fault handling
    flash_crash_t crash;
    flash_planner_t planner;
    flash_progress_t progress;
    flash_print_t print;

#if ENABLED(CANCEL_OBJECTS)
    uint32_t canceled_objects;
#endif
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
#if ENABLED(CANCEL_OBJECTS)
    FLASH_SAVE(state.canceled_objects, state_buf.canceled_objects);
#endif

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
#if ENABLED(CANCEL_OBJECTS)
    FLASH_LOAD(state.canceled_objects, state_buf.canceled_objects);
#endif
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
    if (state_buf.print.odometer_e_start >= Odometer_s::instance().get(Odometer_s::axis_t::E)) {
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

    oProgressData.oPercentDone.mSetValue(state_buf.progress.percent_done, state_buf.progress.print_duration);
    oProgressData.oTime2End.mSetValue(state_buf.progress.time_to_end, state_buf.progress.print_duration);
    oProgressData.oTime2Pause.mSetValue(state_buf.progress.time_to_pause, state_buf.progress.print_duration);

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
    ac_fault_state = AcFaultState::Prepared;
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

ResumeState resume_state = ResumeState::Setup;

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
    ac_fault_state = AcFaultState::Inactive;
    resume_state = ResumeState::Setup;
    state_buf.nested_fault = false;
}

/// transition from a nested_fault to a normal fault atomically
static void atomic_finish() {
    HAL_NVIC_DisableIRQ(buddy::hw::acFault.getIRQn());

    marlin_server_powerpanic_finish(state_buf.planner.was_paused);
    atomic_reset();

    HAL_NVIC_EnableIRQ(buddy::hw::acFault.getIRQn());
}

void resume_print(bool auto_recover) {
    assert(state_stored());               // caller is responsible for checking
    assert(marlin_server_printer_idle()); // caller is responsible for checking

    log_info(PowerPanic, "resuming print");
    state_buf.nested_fault = true;
    if (resume_state == ResumeState::Setup && auto_recover) {
        resume_state = ResumeState::Resume;
    }
    marlin_server_powerpanic_resume_loop(state_buf.media_SFN_path, state_buf.crash.sdpos, auto_recover);
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
        if (state_buf.planner.was_paused) {
            // setup the paused state
            resume_state_t resume;
            resume.pos = state_buf.crash.crash_current_position;
            resume.fan_speed = state_buf.planner.fan_speed;
            HOTEND_LOOP() {
                resume.nozzle_temp[e] = state_buf.planner.target_nozzle[e];
            }
            marlin_server_set_resume_data(&resume);
        } else {
#if FAN_COUNT > 0
            thermalManager.set_fan_speed(0, state_buf.planner.fan_speed);
#endif
            HOTEND_LOOP() {
                // setup nozzle temperature
                thermalManager.setTargetHotend(state_buf.planner.target_nozzle[e], e);

                marlin_server_set_temp_to_display(state_buf.planner.target_nozzle[e], e);
            }
        }

        // set bed temperatures
        thermalManager.setTargetBed(state_buf.planner.target_bed);
#if ENABLED(PREVENT_COLD_EXTRUSION)
        thermalManager.extrude_min_temp = state_buf.planner.extrude_min_temp;
        thermalManager.allow_cold_extrude = state_buf.planner.allow_cold_extrude;
#endif
        // planner settings
        planner.settings = state_buf.planner.settings;
        planner.reset_acceleration_rates();
#if HAS_CLASSIC_JERK
        planner.max_jerk = state_buf.planner.max_jerk;
#else
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

        // lift and rehome
        if (TEST(state_buf.crash.axis_known_position, X_AXIS) || TEST(state_buf.crash.axis_known_position, Y_AXIS)) {
            float z_dist = current_position[Z_AXIS] - state_buf.crash.crash_current_position[Z_AXIS];
            float z_lift = z_dist < Z_HOMING_HEIGHT ? Z_HOMING_HEIGHT - z_dist : 0;
            char cmd_buf[24];
            snprintf(cmd_buf, sizeof(cmd_buf), "G28 X Y D R%f", (double)z_lift);
            marlin_server_enqueue_gcode(cmd_buf);
        }

        // canceled objects
#if ENABLED(CANCEL_OBJECTS)
        cancelable.canceled = state_buf.canceled_objects;
#endif

        resume_state = (state_buf.planner.was_paused ? ResumeState::ParkForPause : ResumeState::WaitForHeaters);
        break;

    case ResumeState::WaitForHeaters: {
        // enqueue a proper wait-for-temperature loop
        char cmd_buf[16];
        HOTEND_LOOP() {
            if (state_buf.planner.target_nozzle[e]) {
                snprintf(cmd_buf, sizeof(cmd_buf), "M109 S%d T%d", state_buf.planner.target_nozzle[e], e);
                marlin_server_enqueue_gcode(cmd_buf);
            }
        }
        if (state_buf.planner.target_bed) {
            snprintf(cmd_buf, sizeof(cmd_buf), "M190 S%d", state_buf.planner.target_bed);
            marlin_server_enqueue_gcode(cmd_buf);
        }

        resume_state = ResumeState::Unpark;
        break;
    }

    case ResumeState::Unpark:
        if (planner.movesplanned() || queue.length != 0)
            break;

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
        if (planner.movesplanned() || queue.length != 0)
            break;

        // return to the parking position
        plan_park_move_to_xyz(state_buf.crash.start_current_position,
            NOZZLE_PARK_XY_FEEDRATE, NOZZLE_PARK_Z_FEEDRATE);
        resume_state = ResumeState::Finish;
        break;

    case ResumeState::Finish:
        if (planner.movesplanned() || queue.length != 0)
            break;

        // original planner state
        HOTEND_LOOP() {
            planner.flow_percentage[e] = state_buf.planner.flow_percentage[e];
#if ENABLED(LIN_ADVANCE)
            planner.extruder_advance_K[e] = state_buf.planner.extruder_advance_K[e];
#endif
        }
        gcode.axis_relative = state_buf.planner.axis_relative;

        // restore crash state
        crash_s.start_current_position = state_buf.crash.start_current_position;
        crash_s.crash_current_position = state_buf.crash.crash_current_position;
        crash_s.crash_position = state_buf.crash.crash_position;
        crash_s.segments_finished = state_buf.crash.segments_finished;
        crash_s.leveling_active = state_buf.crash.leveling_active;
        crash_s.inhibit_flags = state_buf.crash.inhibit_flags;
        crash_s.fr_mm_s = state_buf.crash.fr_mm_s;
        crash_s.counter_crash = state_buf.crash.counter_crash;
        crash_s.counter_power_panic = state_buf.crash.counter_power_panic;

        atomic_finish();
        log_info(PowerPanic, "resuming complete");

        resume_state = ResumeState::Error;
        break;

    case ResumeState::Error:
        // fail if marlin_server_powerpanic_finish didn't reset the server loop state
        bsod("resume loop not reset");
    }
}

/// fully reset PP state for a new print
void reset() {
    // reset all internal state
    atomic_reset();

    // also reset print state
    state_buf.print.odometer_e_start = Odometer_s::instance().get(Odometer_s::axis_t::E);

    log_info(PowerPanic, "powerpanic reset");
}

// TODO: move this away
bool has_inverted_axis(const AxisEnum axis) {
    switch (axis) {
    case X_AXIS:
        return has_inverted_x();
    case Y_AXIS:
        return has_inverted_y();
    case Z_AXIS:
        return has_inverted_z();
    case E_AXIS:
        return has_inverted_e();
    default:
        bsod("invalid axis");
    }
}

// Return the physical distance of the requested number of cycles
float distance_per_cycle(const AxisEnum axis, uint8_t cycles) {
    uint32_t res = stepper_axis(axis).microsteps();
    uint32_t steps = (4 * res) * cycles;
    return (float)steps * planner.mm_per_step[axis];
}

// Return the shift required to reach the closest position where MSCNT is zero, moving forward.
float distance_to_reset_point(const AxisEnum axis, uint8_t cycles) {
    // NOTE: we need to ensure to schedule more than "dropsegments" steps in order to move, but this
    // is easily always the case with any non-zero microstepping value and multiples of 4 full steps.
    TMCStepper &stepper = stepper_axis(axis);
    uint32_t res = stepper.microsteps();
    uint32_t steps = (4 * res) * cycles;
    uint32_t mscnt = stepper.MSCNT();
    if (has_inverted_axis(axis))
        mscnt = 1024 - mscnt;
    return (float)mscnt / ((float)res * planner.settings.axis_steps_per_mm[axis])
        + ((float)steps * planner.mm_per_step[axis]);
}

uint8_t shutdown_state = 0;

void shutdown_loop() {
    // shut off devices one-at-a-time in order of power-draw/time saved
    switch (shutdown_state) {
    case 0:
#if HAS_LEDS
        // TODO: needs special function from @radekvana
        leds::SetBrightness(0);
        leds::Set0th(leds::Color(0));
        leds::Set1st(leds::Color(0));
        leds::Set2nd(leds::Color(0));
        leds::TickLoop();
        break;
#else
        ++shutdown_state;
        [[fallthrough]];
#endif

    case 1:
#if 1
        // TODO: needs special function from @radekvana
        ili9488_cmd_dispoff();
        break;
#else
        ++shutdown_state;
        [[fallthrough]];
#endif

    case 2:
        // ethernet (PHY_POWERDOWN)
        break;

    case 3:
        // eeprom
        break;

    case 4:
        // accelerometer?
        break;

    case 5:
        // esp8266?
        break;

    default:
        // no more devices to shutdown, do not increment the sequence
        return;
    }

    // advance the shutdown sequence
    ++shutdown_state;
}

bool shutdown_loop_checked() {
    uint8_t moves = planner.movesplanned();
    if (!moves) {
        // no time to perform any shutdown
        return false;
    }

    // try to run one iteration of the shutdown sequence
    if (moves > 1 || stepper.segment_progress() < 0.5f)
        shutdown_loop();

    // check that no single step takes too long, emit a warning so that we can notice and re-arrange
    // the sequence to avoid stalling (@wavexx consider the move time above if just looking at the
    // move above is not sufficient)
    moves = planner.movesplanned();
    if (!moves)
        log_warning(PowerPanic, "shutdown state %u/%u took too long", ac_fault_state, shutdown_state - 1);

    return moves;
}

std::atomic_bool ac_power_fault = false;
AcFaultState ac_fault_state = AcFaultState::Inactive;

void ac_fault_loop() {
    switch (ac_fault_state) {
    case AcFaultState::Triggered:
        // suspend the helper task
        vTaskSuspend(ac_fault_task);
        log_debug(PowerPanic, "powerpanic loop start");
        // reduce power of motors
        stepperX.rms_current(POWER_PANIC_X_CURRENT, 1);
#if !HAS_PUPPIES()
        stepperE0.rms_current(POWER_PANIC_E_CURRENT, 1);
#else
        // Extruders are on puppy boards that cannot be reached from ISR. These reset on power panic anyway.
#endif

        // extend XY endstops so that we can still retract/park within an interrupted homing move
        soft_endstop.min.x = X_MIN_POS - (X_MAX_POS - X_MIN_POS);
        soft_endstop.max.x = X_MAX_POS + (X_MAX_POS - X_MIN_POS);
        soft_endstop.min.y = Y_MIN_POS - (Y_MAX_POS - Y_MIN_POS);
        soft_endstop.max.y = Y_MAX_POS + (Y_MAX_POS - Y_MIN_POS);

        // resume motion and keep consistent speeds/rates
        crash_s.set_state(Crash_s::RECOVERY);
        planner.reset_acceleration_rates();

        if (!state_buf.nested_fault && !state_buf.planner.was_paused && !state_buf.planner.was_crashed && all_axes_homed()) {
            // retract if we were printing
            plan_move_by(PAUSE_PARK_RETRACT_FEEDRATE, 0, 0, 0, -PAUSE_PARK_RETRACT_LENGTH / planner.e_factor[active_extruder]);
            stepper.start_moving();

            // start powering off complex devices
            shutdown_loop_checked();
        }

        log_info(PowerPanic, "powerpanic triggered");
        ac_fault_state = AcFaultState::Retracting;
        break;

    case AcFaultState::Retracting:
        if (shutdown_loop_checked())
            break;

        disable_e_steppers();

        // align the Z axis by lifting as little as sensibly possible
        if (TEST(state_buf.orig_axis_known_position, Z_AXIS) && TEST(state_buf.crash.axis_known_position, Z_AXIS)) {
            if (!state_buf.nested_fault || current_position[Z_AXIS] != state_buf.planner.z_position) {
                log_debug(PowerPanic, "Z MSCNT start: %d", stepperZ.MSCNT());

                // lift just 1 cycle if already far enough from the print
                float z_dist = current_position[Z_AXIS] - state_buf.crash.crash_current_position[Z_AXIS];
                bool already_lifted = z_dist >= distance_per_cycle(Z_AXIS, POWER_PANIC_Z_LIFT_CYCLES);
                uint8_t cycles = (already_lifted ? 1 : POWER_PANIC_Z_LIFT_CYCLES);
                float z_shift = distance_to_reset_point(Z_AXIS, cycles);
                plan_move_by(POWER_PANIC_Z_FEEDRATE, 0, 0, z_shift);
                stepper.start_moving();

                // continue powering off devices
                shutdown_loop_checked();
            }
        }

        ac_fault_state = AcFaultState::SaveState;
        break;

    case AcFaultState::SaveState:
        if (shutdown_loop_checked())
            break;

        // Z axis is now aligned
        stepperZ.rms_current(POWER_PANIC_Z_CURRENT, 1);
        log_debug(PowerPanic, "Z MSCNT end: %d", stepperZ.MSCNT());
        state_buf.planner.z_position = current_position[Z_AXIS];

        // timer & progress state
        state_buf.progress.print_duration = print_job_timer.duration();
        state_buf.progress.percent_done = oProgressData.oPercentDone.mGetValue();
        state_buf.progress.time_to_end = oProgressData.oTime2End.mGetValue();
        state_buf.progress.time_to_pause = oProgressData.oTime2Pause.mGetValue();

#if ENABLED(CANCEL_OBJECTS)
        state_buf.canceled_objects = cancelable.canceled;
#endif

        log_info(PowerPanic, "powerpanic saving");
        if (state_buf.orig_state != AcFaultState::Prepared) {
            // no prepare was performed for this print yet, prepare the flash now
            flash_data::erase();
            flash_data::fixed_t::save();
        }
        flash_data::state_t::save();
        flash_data::sync();

        // commit odometer trip values
        Odometer_s::instance().force_to_eeprom();

        if (TEST(state_buf.crash.axis_known_position, X_AXIS)) {
#if ENABLED(XY_LINKED_ENABLE)
            // XBuddy has XY-EN linked, so the following move will indirectly enable Y.
            // In order to conserve power and keep Y disabled, set the chopper off time via SPI instead.
            stepperY.toff(0);
#endif
#if ENABLED(SKEW_CORRECTION)
            planner.skew_factor.xy = 0; // avoid triggering Y due to XY skew correction
#endif
            feedrate_mm_s = POWER_PANIC_X_FEEDRATE;
            destination = current_position;

            if (TEST(state_buf.orig_axis_known_position, X_AXIS)) {
                // axis position is currently known, move to the closest endpoint
                destination.x = (current_position.x < X_BED_SIZE / 2 ? X_MIN_POS : X_MAX_POS);
            } else {
                // we might be anywhere, plan some move towards the endstop
                destination.x = current_position.x - (X_MAX_POS - X_MIN_POS);
            }
            prepare_move_to_destination();
            stepper.start_moving();
        }

        log_info(PowerPanic, "powerpanic complete");
        Sound_Play(eSOUND_TYPE::CriticalAlert);
        ac_fault_state = AcFaultState::WaitingToDie;
        break;

    case AcFaultState::WaitingToDie:
        shutdown_loop();

        // hold the system for a while to avoid restarting for a short bursts
        if (!is_panic_signal() && (ticks_ms() - state_buf.fault_stamp) >= POWER_PANIC_HOLD_RST_MS) {
            // time's up: attempt to self-resume by resetting
            sys_reset();
            while (1)
                ;
        }

    case AcFaultState::Inactive:
    case AcFaultState::Prepared:
        // state not reached in this context
        break;
    }
}

void ac_fault_isr() {
    // disable EEPROM writes
    ac_power_fault = true;
    // check if handling the fault is worth it (printer is active or can be resumed)
    if (!state_buf.nested_fault) {
        if (marlin_server_printer_idle() && !marlin_server_printer_paused())
            return;
    }

    // TODO: can be avoided if running at the same priority as STEP_TIMER_PRIO
    CRITICAL_SECTION_START;

    // prevent re-entry
    HAL_NVIC_DisableIRQ(buddy::hw::acFault.getIRQn());
    state_buf.orig_state = ac_fault_state;
    state_buf.fault_stamp = ticks_ms();
    ac_fault_state = AcFaultState::Triggered;

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
        state_buf.planner.was_paused = marlin_server_printer_paused();
        state_buf.planner.was_crashed = crash_s.did_trigger();
    }
    if (!state_buf.planner.was_crashed) {
        // fault occurred outside of a crash: trigger one now to update the crash position
        crash_s.set_state(Crash_s::TRIGGERED_AC_FAULT);
        crash_s.crash_axis_known_position = state_buf.orig_axis_known_position;
    }

    if (!state_buf.nested_fault) {
        const resume_state_t &resume = *marlin_server_get_resume_data();

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
        state_buf.crash.start_current_position = crash_s.start_current_position;
        state_buf.crash.crash_position = crash_s.crash_position;
        state_buf.crash.segments_finished = crash_s.segments_finished;
        state_buf.crash.axis_known_position = crash_s.crash_axis_known_position;
        state_buf.crash.leveling_active = crash_s.leveling_active;
        state_buf.crash.inhibit_flags = crash_s.inhibit_flags;
        state_buf.crash.fr_mm_s = crash_s.fr_mm_s;
        state_buf.crash.counter_crash = crash_s.counter_crash;
        state_buf.crash.counter_power_panic = crash_s.counter_power_panic + 1;

        // save print temperatures
        if (state_buf.planner.was_paused) {
            HOTEND_LOOP() {
                state_buf.planner.target_nozzle[e] = resume.nozzle_temp[e];
            }
            state_buf.planner.fan_speed = resume.fan_speed;
        } else {
            HOTEND_LOOP() {
                state_buf.planner.target_nozzle[e] = thermalManager.degTargetHotend(e);
            }
            state_buf.planner.fan_speed = thermalManager.fan_speed[0];
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
#if ENABLED(LIN_ADVANCE)
            state_buf.planner.extruder_advance_K[e] = planner.extruder_advance_K[e];
#endif
        }
        state_buf.planner.axis_relative = gcode.axis_relative;
        state_buf.planner.settings = planner.settings;
#if HAS_CLASSIC_JERK
        state_buf.planner.max_jerk = planner.max_jerk;
#else
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
#if !HAS_PUPPIES()
    thermalManager.suspend_heatbreak_fan(2000);
#endif

    // stop & disable endstops
    media_print_quick_stop(MEDIA_PRINT_UNDEF_POSITION);
    endstops.enable_globally(false);

    // will continue in the main loop
    xTaskResumeFromISR(ac_fault_task);
    CRITICAL_SECTION_END;
}

bool is_panic_signal() {
    return buddy::hw::acFault.read() == buddy::hw::Pin::State::low;
}

} // namespace power_panic
