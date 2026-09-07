/**
 * @file fsm_preheat_type.hpp
 */

#pragma once

#include "client_fsm_types.hpp"
#include <common/fsm_base_types.hpp>
#include <tool_index.hpp>
#include <bsod/bsod.h>

#include <utility>

/**
 * @brief object to pass preheat data between threads
 */
struct PreheatData {
    using ToolIndex = std::variant<VirtualToolIndex, AllTools>;

    ToolIndex tool;

    PreheatMode mode : 3;
    static_assert(std::to_underlying(PreheatMode::_last) < (1 << 3));

    bool has_return_option : 1;
    bool has_cooldown_option : 1;

    static constexpr PreheatData deserialize(fsm::PhaseData data) {
        return fsm::deserialize_data<PreheatData>(data);
    }

    constexpr fsm::PhaseData serialize() const {
        return fsm::serialize_data(*this);
    }
};

constexpr bool preheat_mode_assume_filament_already_inserted(PreheatMode m) {
    switch (m) {

    case PreheatMode::preheat:
        // Assume we're preheating for a print with an already loaded filament
        // We don't want to get warnings like "the nozzle is not hardened" here
        return true;

    case PreheatMode::purge:
    case PreheatMode::unload:
        return true;

    case PreheatMode::standard_load:
    case PreheatMode::change_load:
    case PreheatMode::autoload:
        return false;
    }

    bsod_unreachable();
}
