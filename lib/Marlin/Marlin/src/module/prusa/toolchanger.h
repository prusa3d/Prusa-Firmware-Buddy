#pragma once
#include <bitset>
#include <inc/MarlinConfigPre.h>
#include <option/has_toolchanger.h>
#include <tool_index.hpp>
#include <utils/variant_utils.hpp>
#include <option/has_indx.h>
#include <option/has_tool_crash_recovery.h>

#if HAS_TOOLCHANGER()
    #include "toolchanger_utils.h"
    #include "bsod.h"
    #include <puppies/PuppyModbus.hpp>
    #if HAS_INDX()
        #include <fsm/nozzle_mismatch_phases.hpp>
    #endif

    #include <module/motion.h>

class PrusaToolChanger : public PrusaToolChangerUtils {
public:
    PrusaToolChanger()
        : PrusaToolChangerUtils() {
    }

    /**
     * @brief Perform a tool-change.
     * It may result in moving the previous tool out of the way and the new tool into place.
     * @warning Only run this from Marlin thread.
     * @param new_tool marlin id of new tool [indexed from 0]
     * @param return_type whether to return to previous position
     * @param return_position where to return to, needed for Z return even if no_return
     * @param z_lift select if printer should do Z lift before moving
     * @param z_return when true, printer will go to return_position.z after toolchange is complete, false will leave Z in last state (possibly lifted by z_lift)
     * @return true if toolchange was successful
     */
    [[nodiscard]] bool tool_change(const std::variant<PhysicalToolIndex, NoTool> new_tool, tool_return_t return_type, xyz_pos_t return_position, tool_change_lift_t z_lift = tool_change_lift_t::full_lift, bool z_return = true);

    float calc_z_raise(tool_return_t return_type, xyz_pos_t return_position, tool_change_lift_t z_lift, bool levelling_active) const;

    #if HAS_TOOL_CRASH_RECOVERY() || HAS_INDX()

    /// Data captured at toolchange start; used by tool-fall crash recovery and
    /// (on INDX) by power-panic-during-toolchange recovery to drive the replay.
    struct ToolchangeReturnData {
        std::variant<PhysicalToolIndex, NoTool> tool { NoTool {} }; ///< Last requested tool (not the tool physically picked)
        tool_return_t return_type {}; ///< How to return after toolchange
        XYZval<float, LogicalPosTag> return_pos {}; ///< Logical return position (used when return_type selects it)
    };

    /// Get last recorded toolchange return data. Used in tool failure recovery.
    const ToolchangeReturnData &return_data() const {
        return return_data_;
    }

    /// Set toolchange return data. Used when recovering from powerpanic through toolcrash.
    void set_return_data(const ToolchangeReturnData &d) {
        return_data_ = d;
    }

    #endif // HAS_TOOL_CRASH_RECOVERY() || HAS_INDX()

    #if PRINTER_IS_PRUSA_XL()
    /**
     * @brief Purges tool by extruding some filament outside of print area and tries to shake it away and wipe it by parking
     */
    bool purge_tool(PhysicalToolIndex tool);
    #endif

    #if HAS_TOOL_CRASH_RECOVERY()

    /**
     * @brief During toolfall recovery (or toolcrash recovery on XL) deselect active tool.
     * @warning Only run this from Marlin thread.
     */
    void crash_deselect_tool();

    /**
     * @brief Disable loop() with automatic toolchange and toolfall detection.
     */
    void toolcheck_disable() {
        if (block_tool_check.exchange(true)) { // Test if already blocked
            bsod("Toolchange loop() already blocked");
        }
    }

    /**
     * @brief Reenable loop() with automatic toolchange and toolfall detection.
     */
    void toolcheck_enable();

    #endif // HAS_TOOL_CRASH_RECOVERY()

    /**
     * @brief Returns calibrated value with the tools docking position
     *
     * @param tool_nr number of tool
     * @param check_calibrated Redscreens if true and the tool is not calibrated
     */
    const xy_float_t get_tool_dock_position(PhysicalToolIndex tool, bool check_calibrated = true);

    /// @returns position just in front of the dock, the tool being ready to be parked
    xy_pos_t tool_park_position(PhysicalToolIndex tool);

    /**
     * @brief Loop that checks toolchanger state.
     * @warning Called only directly from marlin server.
     * @param printing true if currently printing, to not start toolchange spontaneously
     * @param paused true if currently paused (not printing), to not start toolchange spontaneously
     */
    void loop(bool printing, bool paused);

    /**
     * @brief Move to a XY position
     * @param x move to this x [mm]
     * @param y move to this y [mm]
     * @param feedrate use this feedrate [mm/s]
     */
    static void move(const float x, const float y, const feedRate_t feedrate);

    /**
     * @brief Bail out this toolchange fast
     *
     * Used from self-test when moves are stooped to avoid error report from toolchanger.
     *
     * This is sad and desperate way to avoid errors from quick stopped moves during toolchange. Consider situation:
     * - Selftest runs some operation that requires toolchange.
     *   (It cannot call it directly, but rather schedules a gcode to call it async)
     * - Toolchange is on default's task stack
     * - Toolchange schedules a move
     * - Schedule move gest on the default's task stack
     * - Move calls idle
     * - Now selftest button response is on the top of default's stack
     * - Response is abort (we want to cancel everything, disable steppers)
     *
     * There seems to be no nice way how to communicate the quick stop was executed as response to abort.
     * As the toolchange is already being run on the same stack as the response handling, it is already in the middle
     * of the operation. The Abort handling cannot just wait for it to finish as it is being run on the same stack.
     * Here a mature cooperative planning framework would terminate the toolchange task, but we are not that far.
     * Instead we set a bool tell the toolchange it is ok to terminate now without errors.
     */
    inline void quick_stop() {
        quick_stopped = true;
    }

    /**
     * @brief Know if it is safe to move in X and Y.
     * @return true if X and Y are homed
     */
    [[nodiscard]] bool can_move_safely(AxisHomeLevel required_level = AxisHomeLevel::full);

    /**
     * @brief Try to pick any enabled tool, iterating through all of them.
     * Useful when we need some tool picked but don't care which one.
     * @param return_type whether to return to previous position
     * @param return_position where to return to
     * @param z_lift select if printer should do Z lift before moving
     * @param z_return when true, printer will go to return_position.z after toolchange
     * @return true if any tool was successfully picked
     */
    [[nodiscard]] bool pick_any_tool(tool_return_t return_type, xyz_pos_t return_position, tool_change_lift_t z_lift = tool_change_lift_t::full_lift, bool z_return = true);

    #if HAS_INDX()

    /// Phase of the current INDX toolchange, observed by power panic.
    enum class ToolchangePhase : uint8_t {
        none,
        before_lock, ///< tool_change started, lock dwell not yet completed
        after_lock, ///< post-lock dwell completed, exit/return pending
    };

    /// Acquire-ordered load of the current phase (publishes the latest return_data()).
    ToolchangePhase phase() const { return phase_.load(std::memory_order_acquire); }

    /// Continue an INDX toolchange interrupted by power panic.
    ///
    /// active_tool is the tool currently held at PP time (from saved planner state).
    /// rd is the saved ToolchangeReturnData from state_buf at PP time.
    /// phase is passed explicitly from on-flash state (not read from phase_).
    /// Returns true if the toolchange completed successfully, false on failure (caller handles fallback).
    [[nodiscard]] bool recover_pp_toolchange(
        const ToolchangeReturnData &rd,
        std::variant<PhysicalToolIndex, NoTool> active_tool,
        ToolchangePhase phase);

    /**
     * @brief Ensure head locking mechanism is open.
     * Checks head_open flag; if not open, calls open_head().
     * @param tool tool whose dock position to use; NoTool defaults to tool 0
     * @return true if head was already open or open head procedure was successful
     */
    bool ensure_head_open(std::variant<PhysicalToolIndex, NoTool> tool = NoTool {});

    /**
     * @brief Manually park a tool whose dock association the firmware doesn't know.
     *
     * Runs the bump-to-dock → set_active_extruder → persist → park sequence used
     * by the nozzle-mismatch recovery. With no @p tool, drives the nozzle-mismatch
     * FSM (prompt → dock_selection → parking → dock_not_empty) so the user can
     * pick a dock interactively. With @p tool, jumps straight to bump+park on that
     * dock without involving the FSM.
     *
     * @return true on success (tool parked), false on abort/failure
     */
    bool manual_tool_park(std::optional<PhysicalToolIndex> tool = std::nullopt);

    enum class BumpError : uint8_t {
        unsafe_move,
        hit
    };

    /// Try bumping to a @p dock position to see if its empty.
    std::expected<void, BumpError> bump_to_dock(PhysicalToolIndex dock);

    /// Ensures some tool is picked and selected @p dock is empty
    /// @warning throws a fatal error on failure
    void pick_tool_out_of_dock(PhysicalToolIndex dock);

    /**
     * @brief Updates/invalidates the last picked tool (only when not printing if not overriden)
     * @param tool Which tool to write
     * @param override_always do this even if we are printing
     */
    void persist_last_picked_tool(std::variant<PhysicalToolIndex, NoTool> tool, bool override_always = false);

    /// Mark the current print as using a single physical tool (or not): persist that tool once,
    /// then leave the EEPROM last-picked tool frozen for the rest of the print. nullopt = multi-tool.
    void set_single_print_tool(std::optional<PhysicalToolIndex> tool) {
        if (tool) {
            persist_last_picked_tool(*tool, /*override_always=*/true);
        }
        freeze_last_picked_tool_ = tool.has_value();
    }

    /**
     * @brief Check nozzle presence against EEPROM last picked tool.
     *
     * - EEPROM says no tool but nozzle detected -> warn user (tool unexpectedly present)
     * - EEPROM says tool picked but no nozzle detected -> auto-correct to no tool
     *
     * @warning Must be called from the main (default) thread after puppies are ready.
     */
    void check_nozzle_presence_vs_eeprom();

    /**
     * @brief Periodically verify the picked tool's nozzle is still present during print.
     *
     * Compares the in-RAM active tool against the nozzle presence sensor. On mismatch,
     * pauses the print and opens the NozzleMismatch FSM. EEPROM is intentionally not
     * consulted here — it is invalidated during prints.
     *
     * Self-throttled to PRINT_NOZZLE_CHECK_PERIOD_MS; safe to call every cycle.
     * @warning Must be called from the main (default) thread, with block_tool_check clear.
     */
    void check_nozzle_presence_during_print();

    /// @brief Disable nozzle presence checking (e.g. during dock calibration)
    void set_nozzle_check_disabled(bool disabled) { nozzle_check_disabled = disabled; }
    bool is_nozzle_check_disabled() const { return nozzle_check_disabled; }

    uint16_t get_pickup_fail_count() const { return pickup_fail_count; }
    uint16_t get_park_fail_count() const { return park_fail_count; }

    #else
    /**
     * @brief Align the tool locks by bumping the right edge of the printer.
     * @warning Needs to be run from homing routine where motor currents and feedrates are set.
     * @note Needs homing after.
     * @note Does nothing if tool is picked.
     * @return true on success
     */
    [[nodiscard]] bool align_locks();
    #endif
private:
    #if HAS_TOOL_CRASH_RECOVERY() || HAS_INDX()
    ToolchangeReturnData return_data_; ///< Toolchange return state captured for crash/PP recovery
    #endif
    #if HAS_TOOL_CRASH_RECOVERY()
    uint8_t tool_check_fails = 0; ///< Count before toolfall
    static constexpr uint8_t TOOL_CHECK_FAILS_LIMIT = 3; ///< Limit of tool_check_fails before toolfall
    #endif
    std::atomic<bool> block_tool_check = false; ///< When true, block toolchange recursion and (on Dwarf) loop() with toolfall detection
    std::atomic<bool> quick_stopped = false; ///< The current toolchange was quick-stopped

    /**
     * @brief Ensure that X and Y are homed to be able to pick/park.
     * @return true on success, false if move is not safe after an attempt to home
     */
    [[nodiscard]] bool ensure_safe_move();

    #if HAS_INDX()
    /// Release/acquire publication fence over the otherwise non-atomic return_data_:
    /// the writer stores return_data_, then phase_ (release); a reader (PP ISR /
    /// PP save) loads phase_ (acquire) and only then reads return_data_.
    std::atomic<ToolchangePhase> phase_ { ToolchangePhase::none };

    bool head_open = false; ///< True when head locking mechanism is known to be open
    bool nozzle_check_disabled = false; ///< When true, skip nozzle presence vs EEPROM check (session-only, resets on reboot)
    std::atomic<uint16_t> pickup_fail_count = 0; ///< Number of failed pickup attempts since boot
    std::atomic<uint16_t> park_fail_count = 0; ///< Number of failed park attempts since boot

    static constexpr uint32_t PRINT_NOZZLE_CHECK_PERIOD_MS = 5000; ///< Period of in-print nozzle presence check
    uint32_t last_print_nozzle_check_ms = 0; ///< Tick of last in-print nozzle presence check

    bool freeze_last_picked_tool_ = false; ///< Single-tool print: don't invalidate the pinned last-picked tool mid-print

    /**
     * @brief Invalidate XY homing state, forcing a rehome before next toolchange.
     */
    void invalidate_xy_homing();

    /**
     * @brief If the given tool is at a position that would interfere with X homing,
     * move the head out of the way first.
     * @param tool the tool currently picked
     */
    void ensure_tool_homeable(PhysicalToolIndex tool);

    /**
     * @brief Open the head locking mechanism.
     * Moves to dock, wiggles E to align teeth, then fully unlocks.
     * @param tool tool whose dock position to use
     */
    void open_head(PhysicalToolIndex tool);

    /**
     * @brief Wiggle E to align teeth, then partial unlock.
     * Shared by open_head and park_procedure. Must be called with EMotorGuard active.
     */
    void wiggle_and_partial_unlock();

    /**
     * @brief Verify nozzle presence/absence.
     * @param prev_tool index of previous tool (for logs)
     * @param expect_present true after pickup (expect nozzle), false after park (expect no nozzle)
     * @param mode controls the bail-out condition during the wait (see WaitMode)
     * @return true if nozzle state matches expectation
     */
    [[nodiscard]] bool verify_nozzle_state(PhysicalToolIndex prev_tool, bool expect_present, WaitMode mode = WaitMode::default_mode);

    enum class ToolchangeFailureAction { abort,
        retry };

    /**
     * @brief Show toolchange failure dialog and get user decision.
     * @param main_phase the primary Ignore/Retry/Abort prompt
     * @param confirm_abort_phase the abort confirmation prompt
     * @return user's chosen action
     */
    [[nodiscard]] ToolchangeFailureAction handle_toolchange_failure(
        PhaseNozzleMismatch main_phase,
        PhaseNozzleMismatch confirm_abort_phase);

    [[nodiscard]] ToolchangeFailureAction handle_pickup_failure();
    [[nodiscard]] ToolchangeFailureAction handle_park_failure();

    /**
     * @brief Apply common failure side-effects and return the action for caller control flow.
     */
    ToolchangeFailureAction apply_failure_action(ToolchangeFailureAction action);

    /**
     * @brief Pickup tool.
     * @param tool this tool
     * @return true on success
     */
    [[nodiscard]] bool pickup(PhysicalToolIndex tool);

    /**
     * @brief Execute the physical pickup sequence: move to dock, lock nozzle, exit.
     * Does not commit tool state — caller is responsible for state management.
     * @param tool this tool
     * INDX_TODO: Do homing moves to makes sure we dont crash and crash the printer
     */
    bool pickup_procedure(PhysicalToolIndex tool);

    /**
     * @brief Park tool.
     * @param tool this tool
     * @return true on success
     */
    [[nodiscard]] bool park(PhysicalToolIndex tool);

    /**
     * @brief Execute the physical park sequence: move to dock, unlock, release nozzle, exit.
     * Does not commit tool state — caller is responsible for state management.
     * @param tool this tool
     * INDX_TODO: Do homing moves to makes sure we dont crash and crash the printer
     */
    bool park_procedure(PhysicalToolIndex tool);

    /// Commit parked state (NoTool active, persisted, head open, loadcell reset).
    void commit_park(PhysicalToolIndex previous_tool);

    /// Commit successful pickup: clear head_open, log, update odometer and active extruder state.
    void commit_pickup(PhysicalToolIndex tool, bool count_in_odometer, bool force_persist);

    struct FinalToolChangeMoves {
        xyz_pos_t return_position;
        tool_return_t return_type;
        bool levelling_active;
        bool z_return;
        xyz_pos_t tool_offset_diff;
    };

    /// Apply post-pickup return moves (XY unpark, Z return, synchronize); adjusts return_position by tool_offset_diff.
    void final_tool_change_moves(const FinalToolChangeMoves &args);

    /// PP recovery: tool not yet locked; park held tool (if any) and re-run tool_change from scratch.
    [[nodiscard]] bool recover_before_lock(
        const ToolchangeReturnData &rd,
        std::variant<PhysicalToolIndex, NoTool> active_tool);

    /// PP recovery: tool mechanically locked but commit_pickup not yet run; finish the pickup.
    [[nodiscard]] bool finish_pickup_after_lock(const ToolchangeReturnData &rd);

    #else
    /**
     * @brief Check if powerpanic happened.
     * @return true if powerpanic happened and toolchange has to quit immediately
     */
    [[nodiscard]] bool check_emergency_stop();

    /**
     * @brief Pickup tool.
     * @param dwarf this tool
     * @return true on success
     */
    [[nodiscard]] bool pickup(buddy::puppies::Dwarf &dwarf);

    /**
     * @brief Park tool.
     * @param dwarf this tool
     * @return true on success
     */
    [[nodiscard]] bool park(buddy::puppies::Dwarf &dwarf);

    /**
     * @brief Check if steps were skipped during parking.
     * If so, reset crash_s state and do homing.
     * @return true on success, false if rehoming failed
     */
    bool check_skipped_step();

    /**
     * @brief When crash happens during toolchange, triggers toolchanger recovery sequence.
     * Called from tool_change().
     */
    void toolcrash();

    /**
     * @brief When printing and tool falls off, triggers toolchanger recovery sequence.
     * Called from marlin server loop() task.
     */
    void toolfall();

    /**
     * @brief Plan a smooth unparking move towards destination
     * unpark_to() should only be called after pickup() in order to plan a smooth unpark move,
     * mirroring the park operation. Such move only makes sense if the toolhead hasn't stopped
     * for other operations, such as Z compensation.
     * @param destination
     */
    void unpark_to(const xy_pos_t &destination);

    #endif

    /**
     * @brief Compensate the Z offset by the specified amount
     */
    void z_shift(const float diff);
};

extern PrusaToolChanger prusa_toolchanger;

#endif
