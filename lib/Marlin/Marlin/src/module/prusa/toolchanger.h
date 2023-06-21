#pragma once
#include <inc/MarlinConfigPre.h>

#if ENABLED(PRUSA_TOOLCHANGER)
    #include "toolchanger_utils.h"

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
    [[nodiscard]] bool tool_change(const uint8_t new_tool, tool_return_t return_type, xyz_pos_t return_position, tool_change_lift_t z_lift = tool_change_lift_t::full_lift, bool z_return = true);

    /// Structure to remember wanted toolchange result in case of a crash
    struct PrecrashData {
        uint8_t tool_nr;           ///< Marlin id of last requested tool [indexed from 0] (last requested, not the tool physically picked)
        tool_return_t return_type; ///< Last wanted return position

        /**
         * @brief Destination to return to.
         * Linked to return_type.
         * @warning This is logical position! Use return_pos = toLogical(current_position).
         */
        xyz_pos_t return_pos;
    };

    /**
     * @brief Get last wanted state.
     * To be used in tool_change() in tool failure recovery.
     * @return last requested result of a toolchange
     */
    const PrecrashData &get_precrash() const {
        return precrash_data;
    }

    /**
     * @brief Force precrash state.
     * This is to be used when recovering from powerpanic through toolcrash.
     * @param data wanted result of a toolchange
     */
    void set_precrash_state(const PrecrashData &data) {
        precrash_data = data;
    }

    /**
     * @brief Loop that checks toolchanger state.
     * @warning Called only directly from marlin server.
     * @param printing true if currently printing, to not start toolchange spontaneously
     */
    void loop(bool printing);

    /**
     * @brief Move to a XY position
     * @param x move to this x [mm]
     * @param y move to this y [mm]
     * @param feedrate use this feedrate [mm/s]
     */
    static void move(const float x, const float y, const feedRate_t feedrate);

    /**
     * @brief Align the tool locks by bumping the right edge of the printer.
     * @warning Needs to be run from homing routine where motor currents and feedrates are set.
     * @note Needs homing after.
     * @note Does nothing if tool is picked.
     * @return true on success
     */
    [[nodiscard]] bool align_locks();

    /**
     * @brief During toolcrash or toolfall recovery deselect dwarf as if all were parked.
     * @warning Only run this from Marlin thread.
     */
    void crash_deselect_dwarf();

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
    void toolcheck_enable() {
        // Revert selected to active tool
        for (uint8_t i = 0; i < HOTENDS; ++i) {
            getTool(i).set_selected(is_tool_enabled(i) && i == get_active_tool_nr());
        }

        if (!block_tool_check.exchange(false)) { // Test if was blocked
            bsod("Toolchange loop() enabled but not blocked");
        }
    }

private:
    PrecrashData precrash_data = {};                     ///< Remember wanted toolchange result in case of a crash
    std::atomic<bool> block_tool_check = false;          ///< When true, block loop() with automatic toolchange and toolfall detection
    uint8_t tool_check_fails = 0;                        ///< Count before toolfall
    static constexpr uint8_t TOOL_CHECK_FAILS_LIMIT = 3; ///< Limit of tool_check_fails before toolfall

    /**
     * @brief Ensure that X and Y are homed to be able to pick/park.
     * @return true on success, false if move is not safe after an attempt to home
     */
    [[nodiscard]] bool ensure_safe_move();

    /**
     * @brief Check if powerpanic happened.
     * @return true if powerpanic happened and toolchange has to quit immediately
     */
    [[nodiscard]] bool check_powerpanic();

    /**
     * @brief Know if it is safe to move in X and Y.
     * @return true if X and Y are homed
     */
    [[nodiscard]] bool can_move_safely();

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
     * @brief Plan a smooth unparking move towards destination
     * unpark_to() should only be called after pickup() in order to plan a smooth unpark move,
     * mirroring the park operation. Such move only makes sense if the toolhead hasn't stopped
     * for other operations, such as Z compensation.
     * @param destination
     */
    void unpark_to(const xy_pos_t &destination);

    /**
     * @brief Compensate the Z offset by the specified amount
     */
    void z_shift(const float diff);

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
     * @brief Purges too by extruding some filament outside of print area and tries to shake it away and wipe it by parking
     */
    bool purge_tool(buddy::puppies::Dwarf &dwarf);
};

extern PrusaToolChanger prusa_toolchanger;

#endif /*ENABLED(PRUSA_TOOLCHANGER)*/
