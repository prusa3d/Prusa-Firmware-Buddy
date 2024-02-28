/**
 * @file selftest_hot_end_sock_type.hpp
 * @brief selftest filament sensor data to be passed between threads
 */

#pragma once

#include <common/fsm_base_types.hpp>
#include "selftest_sub_state.hpp"

struct SelftestHotEndSockType {
    bool has_sock = false;
    NozzleType nozzle_type = NozzleType::Normal;

    constexpr SelftestHotEndSockType() = default;

    constexpr SelftestHotEndSockType(fsm::PhaseData new_data)
        : SelftestHotEndSockType() {
        Deserialize(new_data);
    }

    constexpr fsm::PhaseData Serialize() const {
        fsm::PhaseData ret = { { has_sock, ftrstd::to_underlying(nozzle_type) } };
        return ret;
    }

    constexpr void Deserialize(fsm::PhaseData new_data) {
        has_sock = new_data[0];
        nozzle_type = static_cast<NozzleType>(new_data[1]);
    }

    constexpr bool operator==(const SelftestHotEndSockType &) const = default;
    constexpr bool operator!=(const SelftestHotEndSockType &other) const = default;

    void Pass() {}
    void Fail() {}
    void Abort() {} // currently not needed
};
