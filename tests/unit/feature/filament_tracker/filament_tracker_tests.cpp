#include <catch2/catch_test_macros.hpp>

#include <feature/filament_tracker/filament_tracker.hpp>

using namespace buddy;

namespace {

FilamentTracker::ExtruderMove calibrated(float commanded_mm) {
    return { .commanded_mm = commanded_mm, .feed_ratio = 1 };
}

} // namespace

TEST_CASE("FilamentTracker") {
    constexpr PhysicalToolIndex pt { 0 };
    constexpr VirtualToolIndex vt { 0 };

    FilamentTracker ft;

    // Should be zeroed out at the beginning
    CHECK(ft.get_retracted_distance(pt) == std::nullopt);
    CHECK(ft.get_extruded_distance(vt) == 0);

    ft.track_extruder_move(vt, calibrated(3));
    CHECK(ft.get_retracted_distance(pt) == 0);
    CHECK(ft.get_extruded_distance(vt) == 3);

    ft.track_extruder_move(vt, calibrated(-2));
    CHECK(ft.get_retracted_distance(pt) == 2);
    CHECK(ft.get_extruded_distance(vt) == 3);

    ft.track_extruder_move(vt, calibrated(2));
    CHECK(ft.get_retracted_distance(pt) == 0);
    CHECK(ft.get_extruded_distance(vt) == 3);

    ft.track_extruder_move(vt, calibrated(1));
    CHECK(ft.get_retracted_distance(pt) == 0);
    CHECK(ft.get_extruded_distance(vt) == 4);

    ft.track_extruder_move(vt, calibrated(-FilamentTracker::extruder_to_nozzle_distance / 2));
    CHECK(ft.get_retracted_distance(pt) == FilamentTracker::extruder_to_nozzle_distance / 2);
    CHECK(ft.get_extruded_distance(vt) == 4);

    ft.track_extruder_move(vt, calibrated(-FilamentTracker::extruder_to_nozzle_distance / 2));
    CHECK(ft.get_retracted_distance(pt) == std::nullopt);
    CHECK(ft.get_extruded_distance(vt) == 4);

    ft.track_extruder_move(vt, calibrated(FilamentTracker::extruder_to_nozzle_distance + 3.5f));
    CHECK(ft.get_retracted_distance(pt) == 0);
    CHECK(ft.get_extruded_distance(vt) == 7 /* .5 */);

    ft.track_extruder_move(vt, calibrated(0.5));
    CHECK(ft.get_retracted_distance(pt) == 0);
    CHECK(ft.get_extruded_distance(vt) == 8);

    ft.track_extruder_move(vt, calibrated(-0.5f));
    CHECK(ft.get_retracted_distance(pt) == 0.5f);
    CHECK(ft.get_extruded_distance(vt) == 8);

    ft.track_extruder_move(vt, calibrated(0.75f));
    CHECK(ft.get_retracted_distance(pt) == 0);
    CHECK(ft.get_extruded_distance(vt) == 8 /* .25 */);
}

TEST_CASE("FilamentTracker reports the length actually fed") {
    constexpr PhysicalToolIndex pt { 0 };
    constexpr VirtualToolIndex vt { 0 };

    // Default Nextruder steps/mm feed 95 % of the commanded length
    const auto move = [](float commanded_mm) {
        return FilamentTracker::ExtruderMove { .commanded_mm = commanded_mm, .feed_ratio = 0.95f };
    };

    FilamentTracker ft;

    ft.track_extruder_move(vt, move(100));
    CHECK(ft.get_extruded_distance(vt) == 95);

    SECTION("retracted distance stays in commanded mm") {
        ft.track_extruder_move(vt, move(-10));
        CHECK(ft.get_retracted_distance(pt) == 10);
        CHECK(ft.get_extruded_distance(vt) == 95);

        // Deretract of the same commanded length counts nothing
        ft.track_extruder_move(vt, move(10));
        CHECK(ft.get_retracted_distance(pt) == 0);
        CHECK(ft.get_extruded_distance(vt) == 95);
    }

    SECTION("many small segments accumulate without loss") {
        // 10500 x 0.01 mm commanded = 105 mm commanded = 99.75 mm fed
        for (int i = 0; i < 10500; ++i) {
            ft.track_extruder_move(vt, move(0.01f));
        }
        CHECK(ft.get_extruded_distance(vt) == 194);
    }

    SECTION("feed ratio 1 counts the commanded length") {
        ft.track_extruder_move(vt, calibrated(100));
        CHECK(ft.get_extruded_distance(vt) == 195);
    }
}
