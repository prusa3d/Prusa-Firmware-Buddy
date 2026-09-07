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
