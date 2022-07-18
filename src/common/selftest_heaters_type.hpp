/**
 * @file selftest_heaters_type.hpp
 * @author Radek Vana
 * @brief selftest fans data to be passed between threads
 * @date 2021-03-01
 */

#pragma once

#include "fsm_base_types.hpp"
#include "selftest_sub_state.hpp"

struct SelftestHeater_t {
    uint8_t progress;
    bool error;
    SelftestSubtestState_t prep_state;
    SelftestSubtestState_t heat_state;

    constexpr SelftestHeater_t(uint8_t progress = 0,
        SelftestSubtestState_t prep_state = SelftestSubtestState_t::undef,
        SelftestSubtestState_t heat_state = SelftestSubtestState_t::undef)
        : progress(progress)
        , error(false)
        , prep_state(prep_state)
        , heat_state(heat_state) {}

    constexpr bool operator==(const SelftestHeater_t &other) const {
        return (progress == other.progress) && (prep_state == other.prep_state)
            && (heat_state == other.heat_state) && (error == other.error);
    }

    constexpr bool operator!=(const SelftestHeater_t &other) const {
        return !((*this) == other);
    }
    void Pass() {
        heat_state = SelftestSubtestState_t::ok;
        prep_state = SelftestSubtestState_t::ok;
        progress = 100;
    }
    void Fail() {
        heat_state = SelftestSubtestState_t::not_good;
        if (prep_state != SelftestSubtestState_t::ok)
            prep_state = SelftestSubtestState_t::not_good; // prepare part passed - dont change that
        progress = 100;
    }

    void Abort() {} // currently not needed
};

struct SelftestHeaters_t {
    SelftestHeater_t noz;
    SelftestHeater_t bed;

    constexpr SelftestHeaters_t(uint8_t noz_progress = 0, uint8_t bed_progress = 0,
        SelftestSubtestState_t noz_prep_state = SelftestSubtestState_t::undef,
        SelftestSubtestState_t noz_heat_state = SelftestSubtestState_t::undef,
        SelftestSubtestState_t bed_prep_state = SelftestSubtestState_t::undef,
        SelftestSubtestState_t bed_heat_state = SelftestSubtestState_t::undef)
        : noz(noz_progress, noz_prep_state, noz_heat_state)
        , bed(bed_progress, bed_prep_state, bed_heat_state) {}

    // SelftestHeater_t is small better to pass it by value
    constexpr SelftestHeaters_t(SelftestHeater_t noz, SelftestHeater_t bed)
        : noz(noz)
        , bed(bed) {}

    constexpr SelftestHeaters_t(fsm::PhaseData new_data)
        : SelftestHeaters_t() {
        Deserialize(new_data);
    }

    constexpr fsm::PhaseData Serialize() const {
        fsm::PhaseData ret = { { uint8_t((noz.progress & 0x7f) | (noz.error ? 0x80 : 0)),
            uint8_t((bed.progress & 0x7f) | (bed.error ? 0x80 : 0)),
            uint8_t(uint8_t(noz.prep_state) | (uint8_t(noz.heat_state) << 2)),
            uint8_t(uint8_t(bed.prep_state) | (uint8_t(bed.heat_state) << 2)) } };
        return ret;
    }

    constexpr void Deserialize(fsm::PhaseData new_data) {
        noz.progress = new_data[0] & 0x7f;
        bed.progress = new_data[1] & 0x7f;
        noz.error = (new_data[0]) >> 7;
        bed.error = (new_data[1]) >> 7;
        noz.prep_state = SelftestSubtestState_t(new_data[2] & 0x03);
        noz.heat_state = SelftestSubtestState_t((new_data[2] >> 2) & 0x03);
        bed.prep_state = SelftestSubtestState_t(new_data[3] & 0x03);
        bed.heat_state = SelftestSubtestState_t((new_data[3] >> 2) & 0x03);
    }

    constexpr bool operator==(const SelftestHeaters_t &other) const {
        return (noz == other.noz) && (bed == other.bed);
    }

    constexpr bool operator!=(const SelftestHeaters_t &other) const {
        return !((*this) == other);
    }
};
