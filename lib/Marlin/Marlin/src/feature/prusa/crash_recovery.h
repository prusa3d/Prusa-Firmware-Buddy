#pragma once
#include "inc/MarlinConfigPre.h"
#include "bsod.h"

#if ENABLED(CRASH_RECOVERY)

    #include <array>
    #include <optional>

    #include "../../module/planner.h"

    /// sanity check
    #if DISABLED(SENSORLESS_HOMING)
        #error "Sensorless homing must be enabled (SENSORLESS_HOMING)."
    #endif

    #if ENABLED(SKEW_CORRECTION_FOR_Z)
        #error "SKEW_CORRECTION_FOR_Z is not currently handled during recovery"
    #endif

static_assert(CRASH_COUNTER_MAX > 1, "Too few crash occurrences. (CRASH_COUNTER_MAX)");
static_assert(CRASH_COUNTER_MAX - 1 <= 256, "More crash occurrences is possible but uint8_t is not enough. (CRASH_COUNTER_MAX)");

/// Singleton that helps with crash detection and recovery
class Crash_s {

    /// types and variables
public:
    /// Crash detection/recovery states
    typedef enum {
        IDLE,               /// initial/disabled state
        PRINTING,           /// printer is working in a usual way (also in pause)
        TRIGGERED_ISR,      /// crash was detected, handling ISR
        TRIGGERED_AC_FAULT, /// crash was triggered during an AC fault
        RECOVERY,           /// crash was detected and recovery is being done
        REPLAY,             /// printer was re-homed and the last G code is being replayed
    } state_t;

    /// Recovery inhibition types
    typedef uint8_t InhibitFlags;
    static const InhibitFlags INHIBIT_PARTIAL_REPLAY = 0b01;
    static const InhibitFlags INHIBIT_XYZ_REPOSITIONING = 0b10;
    static const InhibitFlags INHIBIT_ALL = ~0;

    struct crash_block_t {
        xyze_pos_t start_current_position; /// Starting logical position of the G-code
        pos_t e_position;                  /// Starting physical position of the segment
        int32_t e_steps;                   /// Length of the extrusion in steps
        uint32_t sdpos;                    /// Media location of the interrupted G-code
        uint16_t segment_idx;              /// Aborted segment index
        InhibitFlags inhibit_flags;        /// Inhibit instruction replay flags
        feedRate_t fr_mm_s;                /// Move feedrate
    };

    int16_t home_sensitivity[3]; ///< Homing sensitivity

    /// Temporary crash state (one entry per planner block)
    crash_block_t crash_block[BLOCK_BUFFER_SIZE]; /// circular buffer (in sync with planner's)

    /// Temporary data for the first entry in a logical move
    struct {
        xyze_pos_t start_current_position; /// Starting logical position of the G-code
        uint32_t sdpos;                    /// Media location of the G-code
    } move_start;

    /// Logical g-code state
    struct {
        uint32_t sdpos;             /// Current media location of the G-code
        uint16_t segment_idx;       /// Current segment index
        InhibitFlags inhibit_flags; /// Inhibit instruction replay flags
    } gcode_state;

    uint32_t sdpos;                    /// sdpos of the gcode instruction being aborted
    xyze_pos_t start_current_position; /// absolute logical starting XYZE position of the gcode instruction
    xyze_pos_t crash_current_position; /// absolute logical XYZE position of the crash location
    abce_pos_t crash_position;         /// absolute physical ABCE position of the crash location
    #if ENABLED(LIN_ADVANCE)
    float advance_mm = 0; /// accumulated linear advance mm
    #endif
    xy_uint_t counter_crash; /// 2x uint16_t
    uint16_t counter_power_panic;
    uint16_t segments_finished;
    uint8_t crash_axis_known_position; /// axis state before crashing
    bool leveling_active;              /// state of MBL before crashing
    InhibitFlags inhibit_flags;        /// instruction replay flags before crashing
    feedRate_t fr_mm_s;                /// Replay feedrate

    /// Inhibit partial gcode instruction replay for the current instruction
    void inhibit_gcode_replay(InhibitFlags flags = INHIBIT_ALL) {
        // ensure flags are set before any movement instruction
        assert(gcode_state.segment_idx == 0);
        gcode_state.inhibit_flags = flags;
    }

    bool vars_locked;
    /// Main server loop iteration check
    /// It is reset to false at the beginning of each server iteration cycle and it's used to
    /// detectect incorrect re-entrant usage of the crash handler
    bool loop;

private:
    Crash_s();
    xy_long_t max_period;  ///< max. period of motor phases to trigger crash
    xy_long_t sensitivity; ///< Crash sensitivity
    // Round buffer for crash timestamps.
    // -1 (UINT32_MAX) is treated as undefined.
    // Last crash timestamp is always now (no need to store).
    std::array<std::optional<uint32_t>, CRASH_COUNTER_MAX - 1> crash_timestamps;
    bool active;
    /// State at the moment of the crash
    state_t state;
    uint8_t crash_timestamps_idx; /// index for round buffer
    bool repeated_crash;
    bool stats_saved; /// statistics were saved to EEPROM
    bool enabled;     /// has to cache EEPROM to avoid IRQ deadlock
#if HAS_DRIVER(TMC2130)
    bool filter;      /// use TMC filtering
#endif
    AxisEnum axis_hit;

    /// methods
public:
    void reset();
    void set_state(state_t new_state);
    state_t get_state() { return state; }

    /// Enable/disable crash detection depending on user
    void enable(bool state = true);
    /// Use if user wants crash detection off
    void disable() { enable(false); }
    bool is_enabled() { return enabled; }

    /// Activates crash detection during G-Code processing.
    void activate() { active = true; }

    /// Deactivates crash detection during G-Code processing.
    void deactivate() { active = false; }

    /// \returns true if currently active
    bool is_active() { return active; }

    /// Resets sensitivity and threshold to drivers
    void update_machine();

    /// Return true if trigger() was called (valid saved state is present)
    bool did_trigger() { return state != PRINTING; }

    /// Called by planner when starting a new logical gcode-instruction
    void start_new_gcode(const uint32_t sdpos) {
        gcode_state.sdpos = sdpos;
        gcode_state.segment_idx = 0;
        gcode_state.inhibit_flags = 0;
    };

    void set_sensitivity(xy_long_t sens);
    xy_long_t get_sensitivity() { return sensitivity; }
    void set_max_period(xy_long_t mp);
    xy_long_t get_max_period() { return max_period; }
    /// Sends metrics/logs
    /// Takes axis stored in ISR
    void send_reports();
    void axis_hit_isr(AxisEnum axis) { axis_hit = axis; }
    void write_stat_to_eeprom();
    bool is_repeated_crash() { return repeated_crash; }
    void count_crash();
#if HAS_DRIVER(TMC2130)
    bool get_filter() { return filter; }
    void set_filter(bool on);
#endif

private:
    void stop_and_save();   ///< Stop the planner and update the crash state
    void restore_state();   ///< Restore planner state for a saved crash. *Must* be called one server loop later!
    void resume_movement(); ///< Release the planner from a stop. *Must* be called one server loop later!

    void reset_crash_counter();
    uint32_t clean_history(); /// remove old timestamps, returns number of valid timestamps
    void reset_history();     /// remove all timestamps
    void reset_repeated_crash() {
        if (repeated_crash) {
            repeated_crash = false;
            reset_history();
        }
    }

    /// Mayer's singleton must have part
public:
    static Crash_s &instance();
    Crash_s(const Crash_s &) = delete;
    Crash_s &operator=(const Crash_s &) = delete;

private:
    ~Crash_s() {}
}; /// class Crash_s

/// Singleton that helps with crash detection and recovery
extern Crash_s &crash_s;

/// Deactivates crash detection until destroyed.
class Crash_Temporary_Deactivate {
    bool orig_state;

public:
    Crash_Temporary_Deactivate() {
        orig_state = crash_s.is_active();
        if (orig_state) {
            // Crash state shouldn't be changed within an active block. This check is not perfect nor
            // exhaustive, but it should catch most incorrect usages.
            assert(!planner.get_current_block());

            crash_s.deactivate();
        }
    }
    ~Crash_Temporary_Deactivate() {
        if (orig_state) {
            assert(!planner.get_current_block());
            crash_s.activate();
        }
    }
};

#else

/// Stubs for printers without CRASH_RECOVERY support
static constexpr struct {
    static constexpr bool did_trigger() { return false; }
} crash_s;

#endif // ENABLED(CRASH_RECOVERY)

static constexpr float period_to_speed(uint16_t msteps, uint32_t thrs, const uint32_t steps_per_mm) {
    if (thrs == 0)
        return std::numeric_limits<float>::infinity();
    if (steps_per_mm == 0)
        bsod("0 steps per mm.");
    msteps = _MAX(1, msteps); // 0 msteps is infact 1
    return 12650000.f * msteps / (256.f * thrs * steps_per_mm);
}
