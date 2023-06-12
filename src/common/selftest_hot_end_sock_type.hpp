/**
 * @file selftest_hot_end_sock_type.hpp
 * @brief selftest filament sensor data to be passed between threads
 */

#pragma once

#include "fsm_base_types.hpp"
#include "selftest_sub_state.hpp"

struct SelftestHotEndSockType {
    bool has_sock = false;
    bool prusa_stock_nozzle = true;

    constexpr SelftestHotEndSockType() = default;

    constexpr SelftestHotEndSockType(fsm::PhaseData new_data)
        : SelftestHotEndSockType() {
        Deserialize(new_data);
    }

    constexpr fsm::PhaseData Serialize() const {
        fsm::PhaseData ret = { { has_sock, prusa_stock_nozzle } };
        return ret;
    }

    constexpr void Deserialize(fsm::PhaseData new_data) {
        has_sock = new_data[0];
        prusa_stock_nozzle = new_data[1];
    }

    constexpr bool operator==(const SelftestHotEndSockType &other) const {
        return ((has_sock == other.has_sock) && (prusa_stock_nozzle == other.prusa_stock_nozzle));
    }

    constexpr bool operator!=(const SelftestHotEndSockType &other) const {
        return !((*this) == other);
    }

    void Pass() {}
    void Fail() {}
    void Abort() {} // currently not needed
};
