/**
 * @file crash_recovery_type.hpp
 * @author Radek Vana
 * @brief crash recovery data to be passed between threads
 * @date 2021-10-29
 */

#pragma once

#include "fsm_base_types.hpp"
#include "selftest_sub_state.hpp"

class Crash_recovery_fsm {
public:
    SelftestSubtestState_t x;
    SelftestSubtestState_t y;

    constexpr Crash_recovery_fsm(SelftestSubtestState_t x = SelftestSubtestState_t::undef, SelftestSubtestState_t y = SelftestSubtestState_t::undef)
        : x(x)
        , y(y) {}

    constexpr Crash_recovery_fsm(fsm::PhaseData new_data)
        : Crash_recovery_fsm() {
        Deserialize(new_data);
    }

    constexpr void set(SelftestSubtestState_t x_state = SelftestSubtestState_t::undef, SelftestSubtestState_t y_state = SelftestSubtestState_t::undef) {
        x = x_state;
        y = y_state;
    }

    constexpr fsm::PhaseData Serialize() const {
        fsm::PhaseData ret = { { uint8_t(x), uint8_t(y) } };
        return ret;
    }

    constexpr void Deserialize(fsm::PhaseData new_data) {
        x = SelftestSubtestState_t(new_data[0]);
        y = SelftestSubtestState_t(new_data[1]);
    }

    bool operator==(const Crash_recovery_fsm &other) const {
        return Serialize() == other.Serialize();
    }

    bool operator!=(const Crash_recovery_fsm &other) const {
        return !((*this) == other);
    }
};

/**
 * @brief Class to transport dwarf states for toolchanger crash between marlin and gui threads.
 */
class Crash_recovery_tool_fsm {
public:
    uint8_t enabled; ///< Mask of enabled dwarves
    uint8_t parked; ///< Mask of parked dwarves

    /**
     * @brief Constructor with dwarf masks or empty.
     * @param enabled mask of enabled dwarves
     * @param parked mask of parked dwarves
     */
    constexpr Crash_recovery_tool_fsm(uint8_t enabled = 0, uint8_t parked = 0)
        : enabled(enabled)
        , parked(parked) {}

    /**
     * @brief Constructor from serialized fsm data.
     * @param new_data serialized data
     */
    constexpr Crash_recovery_tool_fsm(fsm::PhaseData new_data)
        : Crash_recovery_tool_fsm() {
        Deserialize(new_data);
    }

    /**
     * @brief Set dwarf masks.
     * @param enabled mask of enabled dwarves
     * @param parked mask of parked dwarves
     */
    constexpr void set(uint8_t set_enabled = 0, uint8_t set_parked = 0) {
        enabled = set_enabled;
        parked = set_parked;
    }

    /**
     * @brief Serialize internal dwarf masks into fsm data.
     * @return serialized data
     */
    constexpr fsm::PhaseData Serialize() const {
        fsm::PhaseData ret = { { uint8_t(enabled), uint8_t(parked) } };
        return ret;
    }

    /**
     * @brief Set internal dwarf masks from fsm data.
     * @param new_data serialized data
     */
    constexpr void Deserialize(fsm::PhaseData new_data) {
        enabled = static_cast<uint8_t>(new_data[0]);
        parked = static_cast<uint8_t>(new_data[1]);
    }

    /**
     * @brief Compare by serialized data.
     */
    bool operator==(const Crash_recovery_tool_fsm &other) const {
        return Serialize() == other.Serialize();
    }

    /**
     * @brief Default != comparator.
     */
    bool operator!=(const Crash_recovery_tool_fsm &other) const {
        return !((*this) == other);
    }
};
