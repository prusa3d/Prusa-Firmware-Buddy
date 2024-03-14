/**
 * @file selftest_loadcell_type.hpp
 * @author Radek Vana
 * @brief selftest loadcell data to be passed between threads
 * @date 2021-09-29
 */

#pragma once

#include "fsm_base_types.hpp"
#include "selftest_sub_state.hpp"
#include <limits>

struct SelftestLoadcell_t {
    static constexpr uint8_t countdown_undef = 0x7f; // 7 bits for countdown
    uint8_t progress;
    uint8_t countdown;
    int16_t temperature;
    bool pressed_too_soon;
    bool failed; // workaround just to pass it to main selftest

    constexpr SelftestLoadcell_t(uint8_t prog = 0)
        : progress(prog)
        , countdown(countdown_undef)
        , temperature(std::numeric_limits<int16_t>::min())
        , pressed_too_soon(false)
        , failed(false) {}

    constexpr SelftestLoadcell_t(fsm::PhaseData new_data)
        : SelftestLoadcell_t() {
        Deserialize(new_data);
    }

    constexpr fsm::PhaseData Serialize() const {
        fsm::PhaseData ret;
        ret[0] = progress;
        ret[1] = countdown & 0x7f; // 7 bits for countdown
        ret[1] |= pressed_too_soon ? 0x80 : 0x00; // 8th bit for pressed_too_soon
        ret[2] = temperature & 0xff;
        ret[3] = temperature >> 8;
        return ret;
    }

    constexpr void Deserialize(fsm::PhaseData new_data) {
        progress = new_data[0];
        countdown = new_data[1] & 0x7f; // 7 bits for countdown
        pressed_too_soon = (new_data[1] & 0x80) == 0x80; // 8th bit for pressed_too_soon
        temperature = new_data[2] | (new_data[3] << 8);
    }

    constexpr bool operator==(const SelftestLoadcell_t &other) const {
        return ((progress == other.progress) && (countdown == other.countdown) && (pressed_too_soon == other.pressed_too_soon) && (temperature == other.temperature));
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
    } // don't touch countdown and pressed_too_soon
    void Abort() {} // currently not needed
};
