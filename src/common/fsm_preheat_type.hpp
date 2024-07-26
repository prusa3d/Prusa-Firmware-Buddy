/**
 * @file fsm_preheat_type.hpp
 */

#pragma once

#include "client_fsm_types.h"
#include <common/fsm_base_types.hpp>

#include <utility>

/**
 * @brief object to pass preheat data between threads
 */
struct PreheatData {
    PreheatMode mode;
    uint8_t extruder;
    bool has_return_option : 1;
    bool has_cooldown_option : 1;

    static constexpr PreheatData make(PreheatMode mode, uint8_t extruder, RetAndCool_t ret_cool = RetAndCool_t::Neither) {
        return PreheatData {
            .mode = mode,
            .extruder = extruder,
            .has_return_option = bool(std::to_underlying(ret_cool) & std::to_underlying(RetAndCool_t::Return)),
            .has_cooldown_option = bool(std::to_underlying(ret_cool) & std::to_underlying(RetAndCool_t::Cooldown)),
        };
    }

    static constexpr PreheatData deserialize(fsm::PhaseData data) {
        static_assert(sizeof(fsm::PhaseData) >= sizeof(PreheatData));

        PreheatData result;
        memcpy(&result, data.data(), sizeof(PreheatData));
        return result;
    }

    constexpr fsm::PhaseData serialize() const {
        static_assert(sizeof(fsm::PhaseData) >= sizeof(PreheatData));

        fsm::PhaseData result;
        memcpy(result.data(), this, sizeof(PreheatData));
        return result;
    }
};
