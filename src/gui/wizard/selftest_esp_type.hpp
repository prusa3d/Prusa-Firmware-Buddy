/**
 * @file selftest_esp_type.hpp
 * @brief selftest esp data to be passed between threads
 */

#pragma once

#include <common/fsm_base_types.hpp>
#include "selftest_sub_state.hpp"

struct SelftestESP_t {
    uint8_t progress;
    uint8_t current_file;
    uint8_t count_of_files;

    constexpr SelftestESP_t(uint8_t prog = 0, uint8_t current_fl = 0, uint8_t count_of_fls = 0)
        : progress(prog)
        , current_file(current_fl)
        , count_of_files(count_of_fls) {}

    constexpr SelftestESP_t(fsm::PhaseData new_data)
        : SelftestESP_t() {
        Deserialize(new_data);
    }

    constexpr fsm::PhaseData Serialize() const {
        fsm::PhaseData ret = { { progress, current_file, count_of_files } };
        return ret;
    }

    constexpr void Deserialize(fsm::PhaseData new_data) {
        progress = new_data[0];
        current_file = new_data[1];
        count_of_files = new_data[2];
    }

    constexpr bool operator==(const SelftestESP_t &other) const {
        return ((progress == other.progress) && (current_file == other.current_file) && (count_of_files == other.count_of_files));
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
