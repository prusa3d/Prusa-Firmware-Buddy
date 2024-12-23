#pragma once
#include "inc/MarlinConfigPre.h"

#if ENABLED(CRASH_RECOVERY)

    #include <array>
    #include <optional>

    #include "../../module/planner.h"
    #include "crash_recovery_counters.hpp"

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
        IDLE, /// initial/disabled state
        PRINTING, /// printer is working in a usual way (also in pause)
        TRIGGERED_ISR, /// crash was detected, handling ISR
        TRIGGERED_AC_FAULT, /// crash was triggered during an AC fault
        TRIGGERED_TOOLFALL, /// dwarf fell off the toolchanger, not during toolchange, regular crash recovery + tool pickup
        TRIGGERED_TOOLCRASH, /// crash during toolchange, no recovery, just pause and wait for tool pickup
        TRIGGERED_HOMEFAIL, /// couldn't home, no recovery, just rehome
        REPEAT_WAIT, /// waiting for user to repark dwarves or to rehome, skips parking and replay
        RECOVERY, /// crash was detected and recovery is being done
        REPLAY, /// printer was re-homed and the last G code is being replayed
        SELFTEST, /// Selftest is running, do not interfere
    } state_t;

    /// Instruction recovery flags
    typedef uint8_t RecoverFlags;
    static constexpr RecoverFlags RECOVER_NONE = 0; // don't recover *any* axis state if instruction is aborted
    static constexpr RecoverFlags RECOVER_AXIS_STATE = 0b1; // recover XYZ axis state (rehome if needed)
    static constexpr RecoverFlags RECOVER_XY_POSITION = 0b10; // recover XY head position on recovery
    static constexpr RecoverFlags RECOVER_Z_POSITION = 0b100; // recover Z head position on recovery
    static constexpr RecoverFlags RECOVER_PARTIAL_REPLAY = 0b1000; // allow partial segment replay on recovery
    static constexpr RecoverFlags RECOVER_FULL = ~0; // full instruction recovery

    /// Default recovery flags for new instructions
    static constexpr RecoverFlags RECOVER_XYZ_POSITION = RECOVER_XY_POSITION | RECOVER_Z_POSITION;
    static constexpr RecoverFlags RECOVER_XYZ = RECOVER_AXIS_STATE | RECOVER_XYZ_POSITION;
    static constexpr RecoverFlags RECOVER_DEFAULT_FLAGS = RECOVER_XYZ;

    struct crash_block_t {
        xyze_pos_t start_current_position; /// Starting logical position of the G-code
        float e_position; /// Starting physical position of the segment
        int32_t e_msteps; /// Length of the extrusion in mini-steps
        uint32_t sdpos; /// Media location of the interrupted G-code
        uint16_t segment_idx; /// Aborted segment index
        RecoverFlags recover_flags; /// Instruction recovery flags
        feedRate_t fr_mm_s; /// Move feedrate
    };

    int16_t home_sensitivity[3]; ///< Homing sensitivity

    /// Temporary crash state (one entry per planner block)
    crash_block_t crash_block[BLOCK_BUFFER_SIZE]; /// circular buffer (in sync with planner's)

    /// Temporary data for the first entry in a logical move
    struct {
        xyze_pos_t start_current_position; /// Starting logical position of the G-code
        uint32_t sdpos; /// Media location of the G-code
    } move_start;

    /// Logical g-code state
    struct {
        uint32_t sdpos; /// Current media location of the G-code
        uint16_t segment_idx; /// Current segment index
        RecoverFlags recover_flags; /// Instruction recovery flags
    } gcode_state;

    uint32_t sdpos = GCodeQueue::SDPOS_INVALID; ///< sdpos of the gcode instruction being aborted

    /**
     * @brief Check if the new sdpos is valid and set it.
     * @param new_sdpos sdpos to be set
     * @return true if the new sdpos is valid and was set
     */
    bool check_and_set_sdpos(uint32_t new_sdpos) {
        if (new_sdpos != GCodeQueue::SDPOS_INVALID) {
            sdpos = new_sdpos;
            return true;
        }
        return false;
    }

    xyze_pos_t start_current_position; /// absolute logical starting XYZE position of the gcode instruction
    xyze_pos_t crash_current_position; /// absolute logical XYZE position of the crash location
    abce_pos_t crash_position; /// absolute physical ABCE position of the crash location
    #if ENABLED(LIN_ADVANCE)
    float advance_mm = 0; /// accumulated linear advance mm
    #endif

    Crash_s_Counters counters;
    using Counter = Crash_s_Counters::Counter;

    uint16_t segments_finished;
    uint8_t crash_axis_known_position; /// axis state before crashing
    bool leveling_active; /// state of MBL before crashing
    RecoverFlags recover_flags; /// instruction replay flags before crashing
    feedRate_t fr_mm_s; /// Replay feedrate

    /**
     * @brief Set gcode instruction replay flags for the current instruction
     * @param flags Replay flags to set for the current instruction
     */
    void set_gcode_replay_flags(RecoverFlags flags) {
        gcode_state.recover_flags = flags;
    }

    bool vars_locked;

    /// Internal check that we're not doing additional crash recovery steps
    /// until functions on the stack return and we get back to the main loop.
    /// The flag is reset in the main marlin loop.
    bool needs_stack_unwind = false;

private:
    Crash_s();
    xy_long_t max_period; ///< max. period of motor phases to trigger crash
    xy_long_t sensitivity; ///< Crash sensitivity
    // Round buffer for crash timestamps.
    // -1 (UINT32_MAX) is treated as undefined.
    // Last crash timestamp is always now (no need to store).
    std::array<std::optional<uint32_t>, CRASH_COUNTER_MAX - 1> crash_timestamps;
    bool active;
    /// State at the moment of the crash
    state_t state;
    uint8_t crash_timestamps_idx; ///< index for round buffer
    bool repeated_crash;
    bool enabled; ///< has to cache EEPROM to avoid IRQ deadlock
    bool m_axis_is_homing[2]; ///< Axis is sensorlessly homing now
    bool m_enable_stealth[2]; ///< Stealth mode should be enabled if crash detection is disabled
    #if HAS_DRIVER(TMC2130)
    bool filter; /// use TMC filtering
    #endif
    AxisEnum axis_hit;
    bool toolchange_event; ///< True if the event was triggered by TRIGGERED_TOOLCRASH or TRIGGERED_TOOLFALL
    bool toolchange_in_progress; ///< True while changing tools, set externally by the prusa_toolchanger
    bool pretoolchange_leveling; ///< True if leveling was active before toolchange, set externally by the prusa_toolchanger
    bool homefail_z; ///< True if Z axis was homing before homing fail, set externally by homing_failed()

    /// methods
public:
    void start_sensorless_homing_per_axis(const AxisEnum axis);
    void end_sensorless_homing_per_axis(const AxisEnum axis, const bool enable_stealth);
    void reset();

    /**
     * @brief Transition to a new state. Has side effects. Only some transitions are allowed.
     * @param new_state set this
     */
    void set_state(state_t new_state);

    /**
     * @brief Get crash state.
     * @return current state
     */
    state_t get_state() const { return state; }

    /// Enable/disable crash detection depending on user
    void enable(bool state = true);
    /// Use if user wants crash detection off
    void disable() { enable(false); }
    bool is_enabled() { return enabled; }

    /// Activates crash detection during G-Code processing.
    void activate() { active = true; }

    /// Deactivates crash detection during G-Code processing.
    void deactivate() { active = false; }

    /// Return true if crash is currently active
    bool is_active() { return active; }

    /// Return true if trigger() was called (valid saved state is present)
    bool did_trigger() { return state != PRINTING; }

    /// Called by planner when starting a new logical gcode-instruction
    void start_new_gcode(const uint32_t sdpos) {
        gcode_state.sdpos = sdpos;
        gcode_state.segment_idx = 0;
        gcode_state.recover_flags = RECOVER_DEFAULT_FLAGS;
    };

    void set_sensitivity(xy_long_t sens);
    xy_long_t get_sensitivity() { return sensitivity; }

    void set_max_period(xy_long_t mp);
    xy_long_t get_max_period() { return max_period; }

    /// Sends metrics/logs
    /// Takes axis stored in ISR
    void send_reports();
    void axis_hit_isr(AxisEnum axis) { axis_hit = axis; }
    bool is_repeated_crash() { return repeated_crash; }
    void count_crash();
    #if HAS_DRIVER(TMC2130)
    bool get_filter() { return filter; }
    void set_filter(bool on);
    #endif

    /**
     * @brief Whether the crash event involves toolchanger and all dwarves need to be picked up.
     * @return true if it involves toolchanger
     */
    bool is_toolchange_event() { return toolchange_event; }

    /**
     * @brief Know if toolchange is in progress.
     * @return true if toolchange is currently in progress
     */
    bool is_toolchange_in_progress() { return toolchange_in_progress; }

    /**
     * @brief Set when changing tools, when regular crash recovery would be dangerous.
     * @param toolchange_in_progress_ true when changing tools
     * @param pretoolchange_leveling_ true if leveling was active before toolchange
     */
    void set_toolchange_in_progress(bool toolchange_in_progress_, bool pretoolchange_leveling_ = false) {
        if (toolchange_in_progress_) {
            // Set levelling first to be valid all time when toolchange_in_progress == true
            pretoolchange_leveling = pretoolchange_leveling_;
        }
        toolchange_in_progress = toolchange_in_progress_;
    }

    /**
     * @brief Set that homing failed during Z.
     * This marks that recovery should also home Z axis.
     */
    void set_homefail_z() { homefail_z = true; }

    /**
     * @brief Know if homing failed during Z.
     * And recovery should also home Z axis.
     * @return true if homing failed during Z
     */
    bool is_homefail_z() const { return homefail_z; }

    /**
     * @brief Ensure the current call is never affected by replay.
     * Some functions need to ensure that all further planned moves are performed entirely, without being
     * affected by replay and fail silently. Call this function to ensure this is the case, or fail loudly.
     */
    void not_for_replay();

private:
    void update_machine();
    void stop_and_save(); ///< Stop the planner and update the crash state
    void restore_state(); ///< Restore planner state for a saved crash. *Must* be called one server loop later!
    void resume_movement(); ///< Release the planner from a stop. *Must* be called one server loop later!

    uint32_t clean_history(); /// remove old timestamps, returns number of valid timestamps
    void reset_history(); /// remove all timestamps
    void reset_repeated_crash() {
        if (repeated_crash) {
            repeated_crash = false;
            reset_history();
        }
    }

    void set_homing_sensitivity(const AxisEnum axis);

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
    [[nodiscard]] Crash_Temporary_Deactivate() {
        orig_state = crash_s.is_active();
        if (orig_state) {
            // Crash state shouldn't be changed while moving.
            assert(!planner.processing());
            crash_s.deactivate();
        }
    }
    ~Crash_Temporary_Deactivate() {
        if (orig_state) {
            // Restore previous state, as long as we ensure the motion has been stopped/is stopping
            // NOTE: that the assertion order is important if the block is aborted while checking!
            assert(!planner.processing() || planner.draining());
            crash_s.activate();
        }
    }

    /**
     * @brief Whether the crash detection was originally enabled.
     * @return true if it was enabled
     */
    [[nodiscard]] bool get_orig_state() const { return orig_state; }
};

#else

/// Stubs for printers without CRASH_RECOVERY support
static constexpr struct {
    static constexpr bool did_trigger() { return false; }
} crash_s;

#endif // ENABLED(CRASH_RECOVERY)
