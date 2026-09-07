/**
 * @file selftest_heaters_type.hpp
 * @author Radek Vana
 * @brief selftest heaters data to be passed between threads
 * @date 2021-03-01
 */

#pragma once

#include <common/fsm_base_types.hpp>
#include "selftest_sub_state.hpp"
#include "marlin_server_extended_fsm_types.hpp"
#include <utility_extensions.hpp>

#include <expected>
#include <variant>

struct __attribute__((packed)) SelftestHeater_t {
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

    constexpr bool operator==(const SelftestHeater_t &other) const = default;
    constexpr bool operator!=(const SelftestHeater_t &other) const = default;

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

    StrongIndexArray<SelftestHeater_t, PhysicalToolIndex::count, PhysicalToolIndex, PhysicalToolIndex::to_raw_static> noz;
    SelftestHeater_t bed;

    std::underlying_type_t<TestedParts> tested_parts { 0 };

    constexpr SelftestHeaters_t() {}

    bool operator==(SelftestHeaters_t const &) const = default;
};

constexpr std::underlying_type_t<SelftestHeaters_t::TestedParts> to_one_hot(SelftestHeaters_t::TestedParts p) {
    return 1 << std::to_underlying(p);
}

namespace heaters_selftest {

// Failure payloads of the INDX nozzle heater test (protection-verdict mode); the measured
// value behind the verdict is carried along so support can diagnose from a screenshot.

/// Thermal model / thermal runaway / heater watch trip
struct ThermalProtectionTripped {
    uint16_t error_code; ///< ErrCode of the intercepted trip
};
struct CoilOvercurrent {
    uint16_t current_ma; ///< peak coil current
};
struct CoilUndercurrent {
    uint16_t current_ma; ///< peak coil current
};
/// Target temperature not reached within the deadline
struct HeatTimeout {
    uint16_t temperature_c; ///< nozzle temperature when the deadline hit
};

/// Sent via PhaseData for the nozzle_failed_dialog phase (must fit in 4 bytes).
/// monostate = failed without a recorded reason; the dialog shows a generic text.
using NozzleError = std::variant<std::monostate, ThermalProtectionTripped, CoilOvercurrent, CoilUndercurrent, HeatTimeout>;
static_assert(sizeof(NozzleError) <= 4);

/// Nozzle test verdict: ok, or why it failed
using NozzleResult = std::expected<void, NozzleError>;

} // namespace heaters_selftest

// Live data for the gcode-based heater selftest (M1987). There is a single physical hotend,
// so unlike the multi-tool SelftestHeaters_t it carries just one nozzle + one bed sub-result.
struct HeatersSelftestData : public FSMExtendedData {
    SelftestHeater_t noz;
    SelftestHeater_t bed;

    constexpr HeatersSelftestData() = default;
    bool operator==(const HeatersSelftestData &) const = default;
};
