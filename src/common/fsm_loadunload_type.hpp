/**
 * @file fsm_loadunload_type.hpp
 * @brief progress data to be passed between threads
 * meant to be used with DialogStateful and its children (on GUI side)
 * usually using FSM_notifier on server side, but can be used directly too
 */

#pragma once

#include "client_fsm_types.h"
#include "fsm_base_types.hpp"
#include <utils/utility_extensions.hpp>

struct ProgressSerializerLoadUnload {
    LoadUnloadMode mode;
    uint8_t progress;

    constexpr ProgressSerializerLoadUnload(LoadUnloadMode mode, uint8_t progress = 0)
        : mode(mode)
        , progress(progress) {}

    constexpr ProgressSerializerLoadUnload(fsm::PhaseData new_data) {
        Deserialize(new_data);
    }

    constexpr fsm::PhaseData Serialize() const {
        fsm::PhaseData ret = { { ftrstd::to_underlying(mode), progress } };
        return ret;
    }

    constexpr void Deserialize(fsm::PhaseData new_data) {
        mode = LoadUnloadMode(new_data[0]);
        progress = new_data[1];
    }

    constexpr bool operator==(const ProgressSerializerLoadUnload &other) const {
        return mode == other.mode && progress == other.progress;
    }

    constexpr bool operator!=(const ProgressSerializerLoadUnload &other) const {
        return !((*this) == other);
    }
};
