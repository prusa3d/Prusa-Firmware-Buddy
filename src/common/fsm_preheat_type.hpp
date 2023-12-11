/**
 * @file fsm_preheat_type.hpp
 */

#pragma once

#include "client_fsm_types.h"
#include <common/fsm_base_types.hpp>

/**
 * @brief object to pass preheat data between threads
 */
class PreheatData {
    static constexpr unsigned mode_digits = 4;
    static constexpr uint8_t mode_mask = (1 << mode_digits) - 1;
    static constexpr uint8_t return_option_digit_offset = 6;
    static constexpr uint8_t return_option_mask = 1 << return_option_digit_offset;
    static constexpr uint8_t cooldown_option_digit_offset = 7;
    static constexpr uint8_t cooldown_option_mask = 1 << cooldown_option_digit_offset;
    uint8_t mode : mode_digits;
    bool has_return_option : 1;
    bool has_cooldown_option : 1;

public:
    constexpr PreheatData(PreheatMode mode, RetAndCool_t ret_cool = RetAndCool_t::Neither)
        : mode(uint8_t(mode))
        , has_return_option(ret_cool == RetAndCool_t::Return || ret_cool == RetAndCool_t::Both)
        , has_cooldown_option((ret_cool == RetAndCool_t::Cooldown || ret_cool == RetAndCool_t::Both) && Mode() == PreheatMode::None) {}
    constexpr PreheatData(fsm::PhaseData data)
        : PreheatData(GetMode(data[0]), GetRetAndCool(data[0])) {}

    constexpr PreheatMode Mode() const { return PreheatMode(mode); }
    constexpr bool HasReturnOption() const { return has_return_option; }
    constexpr bool HasCooldownOption() const { return has_cooldown_option; }
    constexpr RetAndCool_t RetAndCool() {
        return GetRetAndCool(serialize()[0]);
    }
    constexpr fsm::PhaseData serialize() const {
        fsm::PhaseData ret = {};
        ret[0] = mode;
        ret[0] |= uint8_t(has_return_option) << return_option_digit_offset;
        ret[0] |= uint8_t(has_cooldown_option) << cooldown_option_digit_offset;
        return ret;
    }

    // conversionfunctions for ctors etc
    static constexpr bool GetReturnOption(uint8_t data) {
        return data & return_option_mask;
    }
    static constexpr bool GetCooldownOption(uint8_t data) {
        return data & cooldown_option_mask;
    }
    static constexpr RetAndCool_t GetRetAndCool(uint8_t data) {
        const bool has_ret = GetReturnOption(data);
        const bool has_cool = GetCooldownOption(data);
        if (has_ret && has_cool) {
            return RetAndCool_t::Both;
        }
        if (has_ret) {
            return RetAndCool_t::Return;
        }
        if (has_cool) {
            return RetAndCool_t::Cooldown;
        }
        return RetAndCool_t::Neither;
    }
    static constexpr PreheatMode GetMode(uint8_t data) {
        return PreheatMode((data & mode_mask) <= uint8_t(PreheatMode::_last) ? data & mode_mask : uint8_t(PreheatMode::None));
    }
};
