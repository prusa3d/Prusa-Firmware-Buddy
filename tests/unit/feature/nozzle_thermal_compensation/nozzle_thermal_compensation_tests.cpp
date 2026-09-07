#include <feature/nozzle_thermal_compensation/nozzle_thermal_compensation.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using namespace buddy::nozzle_thermal_compensation;

namespace {

constexpr float tolerance = 0.000001f;

/// Guards the model staying foldable at compile time, since it is evaluated on the g-code path
static_assert(elongation_vs_0c_mm(0) == 0);
static_assert(elongation_vs_reference_mm(reference_temperature_c) == 0);

/// What reaches the carriage for a point probed at `tap` and printed at `print`.
constexpr float net_correction_mm(float tap, float print) {
    return elongation_vs_reference_mm(print) - elongation_vs_reference_mm(tap);
}

} // namespace

TEST_CASE("nozzle_thermal_compensation::elongation_vs_0c_mm") {
    CHECK_THAT(elongation_vs_0c_mm(0), Catch::Matchers::WithinAbs(0, tolerance));

    // 21 mm of brass at 1.95e-5 /K is 0.41 um/K
    CHECK_THAT(elongation_vs_0c_mm(100), Catch::Matchers::WithinAbs(0.04095f, tolerance));
    CHECK_THAT(elongation_vs_0c_mm(170), Catch::Matchers::WithinAbs(0.069615f, tolerance));
    CHECK_THAT(elongation_vs_0c_mm(250), Catch::Matchers::WithinAbs(0.102375f, tolerance));

    SECTION("Below zero cannot shrink the nozzle") {
        // Sub-zero readings are a sensor fault, not a shorter nozzle
        CHECK_THAT(elongation_vs_0c_mm(-40), Catch::Matchers::WithinAbs(0, tolerance));
    }
}

TEST_CASE("nozzle_thermal_compensation::elongation_vs_reference_mm") {
    SECTION("Nothing to correct at the reference temperature") {
        CHECK_THAT(elongation_vs_reference_mm(reference_temperature_c), Catch::Matchers::WithinAbs(0, tolerance));
    }

    SECTION("Hotter than the reference lengthens the nozzle") {
        CHECK_THAT(elongation_vs_reference_mm(250), Catch::Matchers::WithinAbs(0.03276f, tolerance));
        CHECK_THAT(elongation_vs_reference_mm(215), Catch::Matchers::WithinAbs(0.0184275f, tolerance));

        // PA and PC probe well above the reference
        CHECK_THAT(elongation_vs_reference_mm(260), Catch::Matchers::WithinAbs(0.036855f, tolerance));
    }

    SECTION("Colder than the reference shortens it, and says so") {
        // nozzle_cleaner_lite probes around 150 for PLA. A colder nozzle really is shorter, so
        // the term is negative rather than clamped - the tip needs the carriage lower.
        CHECK_THAT(elongation_vs_reference_mm(150), Catch::Matchers::WithinAbs(-0.00819f, tolerance));
        CHECK_THAT(elongation_vs_reference_mm(0), Catch::Matchers::WithinAbs(-0.069615f, tolerance));
    }
}

TEST_CASE("nozzle_thermal_compensation net correction") {
    SECTION("The reference cancels out - only tap-to-print matters") {
        // The canonical XL case, levelled at the reference temperature
        CHECK_THAT(net_correction_mm(170, 250), Catch::Matchers::WithinAbs(elongation_vs_0c_mm(250) - elongation_vs_0c_mm(170), tolerance));

        // Probed cold by the nozzle cleaner, printed at PLA temperature
        CHECK_THAT(net_correction_mm(150, 215), Catch::Matchers::WithinAbs(elongation_vs_0c_mm(215) - elongation_vs_0c_mm(150), tolerance));

        // PA: probes hot, prints hotter still. The same 50 K span as PLA gives the same
        // correction despite both absolute temperatures being far higher.
        CHECK_THAT(net_correction_mm(260, 310), Catch::Matchers::WithinAbs(elongation_vs_0c_mm(310) - elongation_vs_0c_mm(260), tolerance));
        CHECK_THAT(net_correction_mm(260, 310), Catch::Matchers::WithinAbs(net_correction_mm(170, 220), tolerance));
    }

    SECTION("A 50 K span is about 20 um whatever it starts from") {
        CHECK_THAT(net_correction_mm(170, 220), Catch::Matchers::WithinAbs(0.0204750f, tolerance));
    }

    SECTION("Probing and printing at the same temperature needs no correction") {
        CHECK_THAT(net_correction_mm(215, 215), Catch::Matchers::WithinAbs(0, tolerance));
        CHECK_THAT(net_correction_mm(150, 150), Catch::Matchers::WithinAbs(0, tolerance));
    }
}
