/**
 * @file selftest_fsensor_type.hpp
 * @author Radek Vana
 * @brief selftest filament sensor data to be passed between threads
 * @date 2021-09-29
 */

#pragma once

#include <common/fsm_base_types.hpp>
#include "selftest_sub_state.hpp"

struct SelftestFSensor_t {
    bool inserted;

    constexpr SelftestFSensor_t(bool inserted = false)
        : inserted(inserted) {}

    constexpr SelftestFSensor_t(fsm::PhaseData new_data)
        : SelftestFSensor_t() {
        Deserialize(new_data);
    }

    constexpr fsm::PhaseData Serialize() const {
        fsm::PhaseData ret = { { inserted } };
        return ret;
    }

    constexpr void Deserialize(fsm::PhaseData new_data) {
        inserted = new_data[0];
    }

    constexpr bool operator==(const SelftestFSensor_t &other) const {
        return ((inserted == other.inserted));
    }

    constexpr bool operator!=(const SelftestFSensor_t &other) const {
        return !((*this) == other);
    }

    void Pass() {}
    void Fail() {}
    void Abort() {} // currently not needed
};
