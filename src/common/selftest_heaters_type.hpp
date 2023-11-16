/**
 * @file selftest_heaters_type.hpp
 * @author Radek Vana
 * @brief selftest fans data to be passed between threads
 * @date 2021-03-01
 */

#pragma once

#include "fsm_base_types.hpp"
#include "selftest_sub_state.hpp"
#include "inc/MarlinConfig.h"
#include "marlin_server_extended_fsm_data.hpp"
#include <utility_extensions.hpp>

struct SelftestHeater_t {
    uint8_t progress;
    bool heatbreak_error;
    SelftestSubtestState_t prep_state;
    SelftestSubtestState_t heat_state;

    constexpr SelftestHeater_t(uint8_t progress = 0,
        SelftestSubtestState_t prep_state = SelftestSubtestState_t::undef,
        SelftestSubtestState_t heat_state = SelftestSubtestState_t::undef)
        : progress(progress)
        , heatbreak_error(false)
        , prep_state(prep_state)
        , heat_state(heat_state) {}

    constexpr bool operator==(const SelftestHeater_t &other) const {
        return (progress == other.progress) && (prep_state == other.prep_state)
            && (heat_state == other.heat_state) && (heatbreak_error == other.heatbreak_error);
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
        if (prep_state != SelftestSubtestState_t::ok) {
            prep_state = SelftestSubtestState_t::not_good; // prepare part passed - dont change that
        }
        progress = 100;
    }

    void Abort() {} // currently not needed
};

struct SelftestHeaters_t : public FSMExtendedData {
public:
    // class to be converted to one_hot coding
    enum class TestedParts : uint8_t {
        noz = 1,
        bed = 2,
    };

    std::array<SelftestHeater_t, HOTENDS> noz;
    SelftestHeater_t bed;

    std::underlying_type_t<TestedParts> tested_parts { 0 };

    constexpr SelftestHeaters_t() {}

    bool operator==(SelftestHeaters_t const &other) const {
        return noz == other.noz && bed == other.bed && tested_parts == other.tested_parts;
    }
};

constexpr std::underlying_type_t<SelftestHeaters_t::TestedParts> to_one_hot(SelftestHeaters_t::TestedParts p) {
    return 1 << ftrstd::to_underlying(p);
}
