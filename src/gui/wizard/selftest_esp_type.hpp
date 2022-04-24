/**
 * @file selftest_esp_type.hpp
 * @brief selftest esp data to be passed between threads
 */

#pragma once

#include "fsm_base_types.hpp"
#include "selftest_sub_state.hpp"

struct SelftestESP_t {
    uint8_t progress;
    uint8_t ver_0;
    uint8_t ver_1;
    uint8_t ver_2;

    constexpr SelftestESP_t(uint8_t prog = 0, uint8_t v0 = 0, uint8_t v1 = 0, uint8_t v2 = 0)
        : progress(prog)
        , ver_0(v0)
        , ver_1(v1)
        , ver_2(v2) {}

    constexpr SelftestESP_t(fsm::PhaseData new_data)
        : SelftestESP_t() {
        Deserialize(new_data);
    }

    constexpr fsm::PhaseData Serialize() const {
        fsm::PhaseData ret = { { progress, ver_0, ver_1, ver_2 } };
        return ret;
    }

    constexpr void Deserialize(fsm::PhaseData new_data) {
        progress = new_data[0];
        ver_0 = new_data[1];
        ver_1 = new_data[2];
        ver_2 = new_data[3];
    }

    constexpr bool operator==(const SelftestESP_t &other) const {
        return ((progress == other.progress) && (ver_0 == other.ver_0) && (ver_1 == other.ver_1) && (ver_2 == other.ver_2));
    }

    constexpr bool operator!=(const SelftestESP_t &other) const {
        return !((*this) == other);
    }

    void Pass() {
        progress = 100;
    }
    void Fail() {
        progress = 100;
    }
    void Abort() {} // currently not needed
};
