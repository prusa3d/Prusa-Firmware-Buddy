/**
 * @file fsm_progress_type.hpp
 * @author Radek Vana
 * @brief progress data to be passed between threads
 * meant to be used with DialogStateful and its children (on GUI side)
 * usually using FSM_notifier on server side, but can be used directly too
 * @date 2021-03-03
 */

#pragma once

#include "fsm_base_types.hpp"

struct ProgressSerializer {
    uint8_t progress;

    constexpr ProgressSerializer(uint8_t progress = 0)
        : progress(progress) {}

    constexpr ProgressSerializer(fsm::PhaseData new_data)
        : ProgressSerializer() {
        Deserialize(new_data);
    }

    constexpr fsm::PhaseData Serialize() const {
        fsm::PhaseData ret = { { progress } };
        return ret;
    }

    constexpr void Deserialize(fsm::PhaseData new_data) {
        progress = new_data[0];
    }

    constexpr bool operator==(const ProgressSerializer &other) const {
        return progress == other.progress;
    }

    constexpr bool operator!=(const ProgressSerializer &other) const {
        return !((*this) == other);
    }
};
