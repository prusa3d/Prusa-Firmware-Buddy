/**
 * @file selftest_heaters_type.hpp
 * @author Radek Vana
 * @brief selftest fans data to be passed between threads
 * @date 2021-03-01
 */

#pragma once

#include "fsm_base_types.hpp"
#include "selftest_sub_state.hpp"

struct SelftestHeaters_t {
    uint8_t noz_progress;
    uint8_t bed_progress;
    SelftestSubtestState_t noz_prep_state;
    SelftestSubtestState_t noz_heat_state;
    SelftestSubtestState_t bed_prep_state;
    SelftestSubtestState_t bed_heat_state;

    constexpr SelftestHeaters_t(uint8_t noz_progress = 0, uint8_t bed_progress = 0,
        SelftestSubtestState_t noz_prep_state = SelftestSubtestState_t::undef,
        SelftestSubtestState_t noz_heat_state = SelftestSubtestState_t::undef,
        SelftestSubtestState_t bed_prep_state = SelftestSubtestState_t::undef,
        SelftestSubtestState_t bed_heat_state = SelftestSubtestState_t::undef)
        : noz_progress(noz_progress)
        , bed_progress(bed_progress)
        , noz_prep_state(noz_prep_state)
        , noz_heat_state(noz_heat_state)
        , bed_prep_state(bed_prep_state)
        , bed_heat_state(bed_heat_state) {}

    constexpr SelftestHeaters_t(fsm::PhaseData new_data)
        : SelftestHeaters_t() {
        Deserialize(new_data);
    }

    constexpr fsm::PhaseData Serialize() const {
        fsm::PhaseData ret = { { noz_progress, bed_progress,
            uint8_t(uint8_t(noz_prep_state) | (uint8_t(noz_heat_state) << 2)),
            uint8_t(uint8_t(bed_prep_state) | (uint8_t(bed_heat_state) << 2)) } };
        return ret;
    }

    constexpr void Deserialize(fsm::PhaseData new_data) {
        noz_progress = new_data[0];
        bed_progress = new_data[1];
        noz_prep_state = SelftestSubtestState_t(new_data[2] & 0x03);
        noz_heat_state = SelftestSubtestState_t((new_data[2] >> 2) & 0x03);
        bed_prep_state = SelftestSubtestState_t(new_data[3] & 0x03);
        bed_heat_state = SelftestSubtestState_t((new_data[3] >> 2) & 0x03);
    }

    constexpr bool operator==(const SelftestHeaters_t &other) const {
        return (noz_progress == other.noz_progress) && (bed_progress == other.bed_progress) && (noz_prep_state == other.noz_prep_state) && (noz_heat_state == other.noz_heat_state) && (bed_prep_state == other.bed_prep_state) && (bed_heat_state == other.bed_heat_state);
    }

    constexpr bool operator!=(const SelftestHeaters_t &other) const {
        return !((*this) == other);
    }
};
