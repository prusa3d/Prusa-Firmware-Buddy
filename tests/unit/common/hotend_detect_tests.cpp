/// @file
/// Unit tests for hotend_detect::detect_impl — see hotend_detect.hpp for the classification rationale.

#include <catch2/catch_test_macros.hpp>
#include <hotend_detect.hpp>

using hotend_detect::detect_impl;
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
