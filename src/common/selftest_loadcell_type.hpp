/**
 * @file selftest_loadcell_type.hpp
 * @author Radek Vana
 * @brief selftest loadcell data to be passed between threads
 * @date 2021-09-29
 */

#pragma once

#include "fsm_base_types.hpp"
#include "selftest_sub_state.hpp"

struct SelftestLoadcell_t {
    static constexpr uint8_t countdown_undef = 255;
    uint8_t progress;
    uint8_t countdown;
    bool pressed_too_soon;
    bool failed; //workaround just to pass it to main selftest

    constexpr SelftestLoadcell_t(uint8_t prog = 0)
        : progress(prog)
        , countdown(countdown_undef)
        , pressed_too_soon(false)
        , failed(false) {}

    constexpr SelftestLoadcell_t(fsm::PhaseData new_data)
        : SelftestLoadcell_t() {
        Deserialize(new_data);
    }

    constexpr fsm::PhaseData Serialize() const {
        fsm::PhaseData ret = { { progress, countdown, pressed_too_soon } };
        return ret;
    }

    constexpr void Deserialize(fsm::PhaseData new_data) {
        progress = new_data[0];
        countdown = new_data[1];
        pressed_too_soon = new_data[2];
    }

    constexpr bool operator==(const SelftestLoadcell_t &other) const {
        return ((progress == other.progress) && (countdown == other.countdown) && (pressed_too_soon == other.pressed_too_soon));
    }

    constexpr bool operator!=(const SelftestLoadcell_t &other) const {
        return !((*this) == other);
    }

    void Pass() {
        progress = 100;
        countdown = countdown_undef;
        pressed_too_soon = false;
    }
    void Fail() {
        progress = 100;
        failed = true;
    }               //don't touch countdown and pressed_too_soon
    void Abort() {} // currently not needed
};
