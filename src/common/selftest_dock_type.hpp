#pragma once

#include <limits>
#include <cassert>
#include <cstring>

#include <common/fsm_base_types.hpp>
#include "selftest_sub_state.hpp"
#include "inc/MarlinConfig.h"

struct SelftestDock_t {
    uint8_t progress;
    SelftestSubtestState_t state;

    constexpr SelftestDock_t(uint8_t prog = 0, SelftestSubtestState_t st = SelftestSubtestState_t::undef)
        : progress(prog)
        , state(st) {}

    constexpr bool operator==(const SelftestDock_t &other) const {
        return (progress == other.progress) && (state == other.state);
    }

    constexpr bool operator!=(const SelftestDock_t &other) const {
        return !((*this) == other);
    }

    void Pass() {
        state = SelftestSubtestState_t::ok;
        progress = 100;
    }

    void Fail() {
        state = SelftestSubtestState_t::not_good;
        progress = 100;
    }

    void Abort() {}

    constexpr fsm::PhaseData Serialize() const {
        fsm::PhaseData ret = { { static_cast<uint8_t>(state) } };
        return ret;
    }
};

class SelftestDocks_t {
    // TODO: This does not handle per dock progress as it would not fit into FSM data :-(

    struct __attribute__((packed)) SerializedData {
        uint8_t current_dock;
        uint8_t total_progress;
        uint16_t states;
    };
    static_assert(sizeof(SerializedData) <= sizeof(fsm::PhaseData), "Selftest docks data fits into Phase data");

public:
    uint8_t current_dock;
    uint8_t total_progress;
    std::array<uint8_t, EXTRUDERS> progresses;
    std::array<SelftestSubtestState_t, EXTRUDERS> states;

    constexpr SelftestDocks_t(uint8_t current_dock, std::array<SelftestDock_t, EXTRUDERS> docks)
        : current_dock(current_dock) {
        uint32_t progress_sum = 0;
        for (uint8_t i = 0; i < EXTRUDERS; ++i) {
            progress_sum += docks[i].progress;
            states[i] = docks[i].state;
        }
        total_progress = progress_sum / EXTRUDERS;
    }

    constexpr SelftestDocks_t(fsm::PhaseData new_data) {
        Deserialize(new_data);
    }

    constexpr fsm::PhaseData Serialize() const {
        union {
            SerializedData serialized;
            fsm::PhaseData ret;
        };

        serialized.current_dock = current_dock;
        serialized.total_progress = total_progress;
        for (uint8_t i = 0; i < EXTRUDERS; ++i) {
            serialized.states |= (static_cast<uint8_t>(states[i]) & 0b11) << 2 * i;
        }

        return ret;
    }

    constexpr void Deserialize(fsm::PhaseData new_data) {
        SerializedData serialized;
        memcpy(&serialized, new_data.data(), sizeof(SerializedData));

        current_dock = serialized.current_dock;
        total_progress = serialized.total_progress;
        for (uint8_t i = 0; i < EXTRUDERS; ++i) {
            // progresses[i] = serialized.progresses[i];
            states[i] = static_cast<SelftestSubtestState_t>((serialized.states >> 2 * i) & 0b11);
        }
    }

    constexpr bool operator==(const SelftestDocks_t &other) const {
        return (progresses == other.progresses) && (total_progress == other.total_progress) && (states == other.states);
    }

    constexpr bool operator!=(const SelftestDocks_t &other) const {
        return !((*this) == other);
    }
};
