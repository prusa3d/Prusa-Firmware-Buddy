#pragma once
#include <inc/MarlinConfigPre.h>

#if ENABLED(PRUSA_TOOLCHANGER)
    #include "toolchanger_utils.h"

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
     */
    bool tool_change(const uint8_t new_tool, tool_return_t return_type = tool_return_t::to_current);

    /**
     * @brief Get last wanted tool.
     * This is tool last requested, not the tool physically picked.
     * To be used in tool_change() in tool failure recovery.
     * @return marlin id of last requested tool [indexed from 0]
     */
    uint8_t get_precrash_tool_nr() const {
        return precrash_tool_nr;
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

private:
    std::atomic<bool> block_tool_check = false;          ///< When true, block autodetect_active_tool() and set picked_dwarf as nullptr
    uint8_t tool_check_fails = 0;                        ///< Count before toolfall
    static constexpr uint8_t TOOL_CHECK_FAILS_LIMIT = 3; ///< Limit of tool_check_fails before toolfall

    uint8_t precrash_tool_nr = MARLIN_NO_TOOL_PICKED; ///< Marlin id of last requested tool [indexed from 0]

    /**
     * @brief Ensure that X and Y are homed to be able to pick/park.
     * @return true on success, false if move is not safe after an attempt to home
     */
    [[nodiscard]] bool ensure_safe_move();

    /**
     * @brief Know if it is safe to move in X and Y.
     * @return true if X and Y are homed
     */
    [[nodiscard]] bool can_move_safely();

    /**
     * @brief Pickup tool.
     * @param dwarf this tool
     * @param diff_z nozzle offset difference in Z
     * @return true on success
     */
    [[nodiscard]] bool pickup(buddy::puppies::Dwarf &dwarf, const float diff_z);

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
};

extern PrusaToolChanger prusa_toolchanger;

#endif /*ENABLED(PRUSA_TOOLCHANGER)*/
