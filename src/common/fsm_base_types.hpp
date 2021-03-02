/**
 * @file fsm_base_types.hpp
 * @author Radek Vana
 * @brief selftest data to be passed between threads
 * @date 2021-03-01
 */

#pragma once

#include <array>
#include <stdint.h>

namespace fsm {

using PhaseData = std::array<uint8_t, 5>;

struct BaseData {
    std::array<uint8_t, 6> phase_and_data;
    constexpr uint8_t GetPhase() const { return phase_and_data[0]; }
    constexpr const uint8_t *Get_pData() const { return phase_and_data.cbegin() + 1; }
    constexpr void SetPhase(uint8_t phase) { phase_and_data[0] = phase; }
    void SetData(PhaseData data) { std::copy(data.begin(), data.end(), phase_and_data.begin() + 1); } // std::copy is not const expr until C++20

    constexpr BaseData()
        : phase_and_data({ {} }) {}
    constexpr BaseData(uint8_t phase, PhaseData data)
        : BaseData() {
        SetPhase(phase);
        SetData(data);
    }
};

};
