/// @file
/// Unit tests for hotend_detect::detect_impl — see hotend_detect.hpp for the classification rationale.

#include <catch2/catch_test_macros.hpp>
#include <hotend_detect.hpp>

using hotend_detect::classify_fan_probe;
using hotend_detect::detect_impl;
using hotend_detect::FanProbeInputs;
using hotend_detect::FanProbeThresholds;
using hotend_detect::FanProbeVerdict;
using hotend_detect::Thresholds;

namespace {
// Mirrors the runtime thresholds derived from temptable_1010 (10-bit ADC).
constexpr Thresholds thresholds = {
    .clear_ntc_upper = 741,
    .clear_ntc_lower = 500,
    // From temptable_2005's own extremes: OV(60) = 320 degC, OV(1023) = -30 degC.
    .shorted_max = 60,
    .open_min = 1023,
};
} // namespace

TEST_CASE("clear NTC above the band: not high_temp, no dialog") {
    const auto out = detect_impl({ 900, false }, thresholds);
    CHECK_FALSE(out.is_high_temp);
    CHECK_FALSE(out.dialog_pending);
}

TEST_CASE("clear NTC below the band (hot NTC): not high_temp, no dialog") {
    const auto out = detect_impl({ 300, false }, thresholds);
    CHECK_FALSE(out.is_high_temp);
    CHECK_FALSE(out.dialog_pending);
}

TEST_CASE("clear NTC, stored high_temp: silent downgrade, no dialog") {
    // Regression: raising the dialog here looped forever (the caller overwrites the stored
    // hotend, the dialog flips it back, the next boot re-detects NTC and re-asks).
    const auto out = detect_impl({ 900, true }, thresholds);
    CHECK_FALSE(out.is_high_temp);
    CHECK_FALSE(out.dialog_pending);
}

TEST_CASE("ambiguous, stored standard: stay standard, pop dialog") {
    const auto out = detect_impl({ 620, false }, thresholds);
    CHECK_FALSE(out.is_high_temp);
    CHECK(out.dialog_pending);
}

TEST_CASE("ambiguous, stored high_temp: stay high_temp, no dialog") {
    const auto out = detect_impl({ 660, true }, thresholds);
    CHECK(out.is_high_temp);
    CHECK_FALSE(out.dialog_pending);
}

// An open or shorted probe is a sensor fault, not evidence about the hotend class.

TEST_CASE("shorted probe, stored high_temp: keep high_temp, no dialog") {
    const auto out = detect_impl({ 0, true }, thresholds);
    CHECK(out.is_high_temp);
    CHECK_FALSE(out.dialog_pending);
}

TEST_CASE("open probe, stored high_temp: keep high_temp, no dialog") {
    const auto out = detect_impl({ 1023, true }, thresholds);
    CHECK(out.is_high_temp);
    CHECK_FALSE(out.dialog_pending);
}

TEST_CASE("open/shorted probe, stored standard: keep standard, no dialog") {
    // No dialog either way: the user cannot resolve a broken sensor by answering a question,
    // and MINTEMP/MAXTEMP is the mechanism that reports it.
    CHECK_FALSE(detect_impl({ 0, false }, thresholds).is_high_temp);
    CHECK_FALSE(detect_impl({ 0, false }, thresholds).dialog_pending);
    CHECK_FALSE(detect_impl({ 1023, false }, thresholds).is_high_temp);
    CHECK_FALSE(detect_impl({ 1023, false }, thresholds).dialog_pending);
}

TEST_CASE("open/short thresholds are inclusive") {
    CHECK(detect_impl({ 60, true }, thresholds).is_high_temp);
    CHECK(detect_impl({ 1023, true }, thresholds).is_high_temp);
}

TEST_CASE("just inside the open/short limits still classifies normally") {
    // 61 is a legitimate very-hot NTC; 1022 a legitimate very-cold one. Neither is a fault,
    // so both must reach the clear-NTC branch and demote a stored HT as before.
    CHECK_FALSE(detect_impl({ 61, true }, thresholds).is_high_temp);
    CHECK_FALSE(detect_impl({ 1022, true }, thresholds).is_high_temp);
}

// --- Phase 1, the print-fan probe -------------------------------------------------------
//
// Deltas below are the proposal's measured 10-bit figures scaled to 12-bit (<<2), so each
// case can be traced back to a real capture on a CORE One rather than to a chosen number.

namespace {
constexpr FanProbeThresholds fan_thresholds = {
    .rise_adc_12bit = 5 << 2, // 5 counts at 10-bit
    // PT1000 at 65 degC. Between a cold PT1000 (~531) and a warm one (626 at 150 degC).
    .warm_nozzle_adc_10bit = 545,
};

constexpr int32_t counts10(int32_t v) { return v << 2; }
} // namespace

TEST_CASE("fan probe: hot NTC shouts") {
    // Measured: +22.4 at 128 degC, +28.9 at 150 degC, +30.7 at 163 degC.
    for (const int32_t rise : { 22, 28, 30 }) {
        CHECK(classify_fan_probe({ counts10(rise), 700 }, fan_thresholds) == FanProbeVerdict::stock_ntc);
    }
}

TEST_CASE("fan probe: hot NTC still detected with a dead fan") {
    // Worst measured corner: dead fan, bottom of the band, cooling into a 65 degC chamber
    // gives +5.3. It must still clear the threshold -- this is the tightest margin in the
    // whole design, 5.3 against 5.
    CHECK(classify_fan_probe({ counts10(5), 724 }, fan_thresholds) == FanProbeVerdict::stock_ntc);
}

TEST_CASE("fan probe: cold PT1000 does not move") {
    // Measured: 0.0 at 20 degC, -0.2 at 30 degC, +0.7 cold-soaked at 38 degC ambient,
    // +1.3 cold-soaked in a 65 degC chamber. None may be read as a rise.
    for (const int32_t drift : { 0, -1, 1, 2 }) {
        CHECK(classify_fan_probe({ drift, 531 }, fan_thresholds) == FanProbeVerdict::inconclusive);
    }
}

TEST_CASE("fan probe: warm PT1000 is caught by the absolute interlock, not the delta") {
    // Measured deltas are only -1.9 to -3.0, far too small to classify on. The absolute
    // reading is what keeps the heat pulse off a nozzle at 150/300/400 degC (626/696/729).
    for (const uint16_t adc : { 626, 696, 729 }) {
        CHECK(classify_fan_probe({ counts10(-2), adc }, fan_thresholds) == FanProbeVerdict::warm_pt1000);
    }
}

TEST_CASE("fan probe: the rise test wins over the interlock") {
    // A hot NTC reads high in absolute terms too, so ordering matters: without it, an NTC at
    // 128 degC (ADC 724) would be called a warm PT1000 and never corrected.
    CHECK(classify_fan_probe({ counts10(22), 724 }, fan_thresholds) == FanProbeVerdict::stock_ntc);
}

TEST_CASE("fan probe: thresholds are inclusive") {
    CHECK(classify_fan_probe({ counts10(5), 400 }, fan_thresholds) == FanProbeVerdict::stock_ntc);
    CHECK(classify_fan_probe({ 0, 545 }, fan_thresholds) == FanProbeVerdict::warm_pt1000);
    CHECK(classify_fan_probe({ counts10(4), 544 }, fan_thresholds) == FanProbeVerdict::inconclusive);
}
