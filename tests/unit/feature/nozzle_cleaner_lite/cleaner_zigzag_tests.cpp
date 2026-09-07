#include <feature/nozzle_cleaner_lite/cleaner_zigzag.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <vector>

using namespace nozzle_cleaner_lite;
using Catch::Approx;

namespace {

/// COREONE geometry: the pad length runs towards lower coordinates.
constexpr Pad core_one_pad {
    .along_near = 196.5f,
    .along_far = 166.5f,
    .across_min = -16.5f,
    .across_max = -13.5f,
};

/// XL geometry: the pad length runs towards higher coordinates.
constexpr Pad xl_pad {
    .along_near = 80.0f,
    .along_far = 110.0f,
    .across_min = -7.45f,
    .across_max = -4.45f,
};

std::vector<ZigZag::Waypoint> run_stroke(ZigZag &zigzag, float along_to) {
    std::vector<ZigZag::Waypoint> waypoints;
    zigzag.begin_stroke(along_to);
    while (const auto waypoint = zigzag.next()) {
        waypoints.push_back(*waypoint);
    }
    return waypoints;
}

/// The full rub of a clean: 5 fast plus 2 slow cycles, each cycle two strokes.
std::vector<std::vector<ZigZag::Waypoint>> run_all_strokes(ZigZag &zigzag, const Pad &pad) {
    std::vector<std::vector<ZigZag::Waypoint>> strokes;
    for (int cycle = 0; cycle < 7; ++cycle) {
        strokes.push_back(run_stroke(zigzag, pad.along_far));
        strokes.push_back(run_stroke(zigzag, pad.along_near));
    }
    return strokes;
}

} // namespace

TEST_CASE("Pad") {
    CHECK(core_one_pad.length() == Approx(30.0f));
    CHECK(core_one_pad.width() == Approx(3.0f));
    CHECK(xl_pad.length() == Approx(30.0f));
}

TEST_CASE("ZigZag start phase picks the band edge") {
    SECTION("phase 0 starts at across_min") {
        const ZigZag zigzag { core_one_pad, 1.5f, 0.0f };
        CHECK(zigzag.start().along == Approx(core_one_pad.along_near));
        CHECK(zigzag.start().across == Approx(core_one_pad.across_min));
    }

    SECTION("half a period starts at across_max") {
        const ZigZag zigzag { core_one_pad, 1.5f, 0.5f };
        CHECK(zigzag.start().across == Approx(core_one_pad.across_max));
    }

    SECTION("a quarter period starts mid band") {
        const ZigZag zigzag { core_one_pad, 1.5f, 0.25f };
        CHECK(zigzag.start().across == Approx(-15.0f));
    }

    SECTION("phases outside [0, 1) are wrapped") {
        const ZigZag wrapped { core_one_pad, 1.5f, 2.5f };
        const ZigZag plain { core_one_pad, 1.5f, 0.5f };
        CHECK(wrapped.start().across == Approx(plain.start().across));
    }

    SECTION("a negative phase mirrors its magnitude") {
        const ZigZag negative { core_one_pad, 1.5f, -0.25f };
        const ZigZag positive { core_one_pad, 1.5f, 0.25f };
        CHECK(negative.start().across == Approx(positive.start().across));
    }
}

TEST_CASE("ZigZag never leaves the pad") {
    for (const float phase : { 0.0f, 0.13f, 0.25f, 0.5f, 0.74f, 0.99f }) {
        for (const Pad &pad : { core_one_pad, xl_pad }) {
            ZigZag zigzag { pad, 1.5f, phase };

            const auto check_in_pad = [&](const ZigZag::Waypoint &waypoint) {
                CHECK(waypoint.across >= pad.across_min - 0.001f);
                CHECK(waypoint.across <= pad.across_max + 0.001f);
                CHECK(waypoint.along >= std::min(pad.along_near, pad.along_far) - 0.001f);
                CHECK(waypoint.along <= std::max(pad.along_near, pad.along_far) + 0.001f);
            };

            check_in_pad(zigzag.start());
            for (const auto &stroke : run_all_strokes(zigzag, pad)) {
                for (const auto &waypoint : stroke) {
                    check_in_pad(waypoint);
                }
            }
        }
    }
}

TEST_CASE("ZigZag strokes end exactly on their target") {
    ZigZag zigzag { core_one_pad, 1.5f, 0.37f };
    for (int cycle = 0; cycle < 3; ++cycle) {
        const auto to_far = run_stroke(zigzag, core_one_pad.along_far);
        REQUIRE_FALSE(to_far.empty());
        CHECK(to_far.back().along == Approx(core_one_pad.along_far));

        const auto to_near = run_stroke(zigzag, core_one_pad.along_near);
        REQUIRE_FALSE(to_near.empty());
        CHECK(to_near.back().along == Approx(core_one_pad.along_near));
    }
}

TEST_CASE("ZigZag advances monotonically along the pad") {
    ZigZag zigzag { core_one_pad, 1.5f, 0.37f };
    float previous = zigzag.start().along;

    for (const auto &stroke : run_all_strokes(zigzag, core_one_pad)) {
        REQUIRE_FALSE(stroke.empty());
        // Strokes towards along_far descend on COREONE, strokes back ascend
        const bool descending = stroke.back().along < previous;
        for (const auto &waypoint : stroke) {
            if (descending) {
                CHECK(waypoint.along <= previous + 0.001f);
            } else {
                CHECK(waypoint.along >= previous - 0.001f);
            }
            previous = waypoint.along;
        }
    }
}

TEST_CASE("ZigZag holds a constant slope over the whole path") {
    // The defining invariant: the across and along coordinates advance in a fixed ratio, so every
    // segment is the same diagonal. This is also what lets the caller approach at
    // start() without the first segment having to drag the nozzle sideways.
    for (const float crossings : { 1.0f, 1.5f, 2.0f }) {
        for (const float phase : { 0.0f, 0.1f, 0.25f, 0.49f }) {
            for (const Pad &pad : { core_one_pad, xl_pad }) {
                ZigZag zigzag { pad, crossings, phase };
                const float slope = pad.width() * crossings / pad.length();

                auto path = std::vector { zigzag.start() };
                for (const auto &stroke : run_all_strokes(zigzag, pad)) {
                    path.insert(path.end(), stroke.begin(), stroke.end());
                }

                for (size_t i = 1; i < path.size(); ++i) {
                    const float along_step = std::abs(path[i].along - path[i - 1].along);
                    const float across_step = std::abs(path[i].across - path[i - 1].across);
                    CHECK(along_step > 0.0001f); // no duplicate waypoints
                    CHECK(std::abs(across_step - slope * along_step) < 0.001f);
                }
            }
        }
    }
}

TEST_CASE("ZigZag turns only at the band edges") {
    ZigZag zigzag { core_one_pad, 1.5f, 0.37f };
    std::vector<ZigZag::Waypoint> path { zigzag.start() };
    for (const auto &stroke : run_all_strokes(zigzag, core_one_pad)) {
        path.insert(path.end(), stroke.begin(), stroke.end());
    }

    // Wherever the drift changes direction it must sit on an edge, otherwise the
    // wave would fold back inside the band and retrace itself.
    int turns = 0;
    for (size_t i = 1; i + 1 < path.size(); ++i) {
        const float before = path[i].across - path[i - 1].across;
        const float after = path[i + 1].across - path[i].across;
        if (before * after < 0.0f) {
            ++turns;
            const bool on_edge = path[i].across == Approx(core_one_pad.across_min)
                || path[i].across == Approx(core_one_pad.across_max);
            CHECK(on_edge);
        }
    }
    // Without this the check above passes vacuously for a wave that never turns
    CHECK(turns > 0);
}

TEST_CASE("ZigZag zigzag coordinate is continuous across strokes") {
    ZigZag zigzag { core_one_pad, 1.5f, 0.37f };
    float previous = zigzag.start().across;

    for (const auto &stroke : run_all_strokes(zigzag, core_one_pad)) {
        for (const auto &waypoint : stroke) {
            // A segment covers at most a half period, so at most the full width
            CHECK(std::abs(waypoint.across - previous) <= core_one_pad.width() + 0.001f);
            previous = waypoint.across;
        }
    }
}

TEST_CASE("ZigZag crossings_per_stroke controls stroke repetition") {
    SECTION("a whole number puts every stroke on the same diagonals") {
        ZigZag zigzag { core_one_pad, 2.0f, 0.0f };
        const auto strokes = run_all_strokes(zigzag, core_one_pad);
        for (size_t i = 2; i < strokes.size(); ++i) {
            REQUIRE(strokes[i].size() == strokes[i - 2].size());
            for (size_t w = 0; w < strokes[i].size(); ++w) {
                CHECK(strokes[i][w].along == Approx(strokes[i - 2][w].along));
                CHECK(strokes[i][w].across == Approx(strokes[i - 2][w].across));
            }
        }
    }

    SECTION("a fraction makes consecutive strokes differ") {
        ZigZag zigzag { core_one_pad, 1.5f, 0.0f };
        const auto strokes = run_all_strokes(zigzag, core_one_pad);
        REQUIRE(strokes.size() >= 3);

        // strokes[2] runs the same direction as strokes[0] but crosses the band
        // the other way, so the turn sits at the same along with the other across edge
        const auto differs = [](const auto &a, const auto &b) {
            if (a.size() != b.size()) {
                return true;
            }
            for (size_t i = 0; i < a.size(); ++i) {
                if (std::abs(a[i].along - b[i].along) > 0.01f || std::abs(a[i].across - b[i].across) > 0.01f) {
                    return true;
                }
            }
            return false;
        };
        CHECK(differs(strokes[0], strokes[2]));
    }

    SECTION("one traverse per stroke turns exactly at the stroke ends") {
        ZigZag zigzag { core_one_pad, 1.0f, 0.0f };
        for (int cycle = 0; cycle < 3; ++cycle) {
            const auto to_far = run_stroke(zigzag, core_one_pad.along_far);
            const auto to_near = run_stroke(zigzag, core_one_pad.along_near);
            // The turn coincides with the stroke end, so no waypoint in between
            REQUIRE(to_far.size() == 1);
            REQUIRE(to_near.size() == 1);
            CHECK(to_far.front().across == Approx(core_one_pad.across_max));
            CHECK(to_near.front().across == Approx(core_one_pad.across_min));
        }
    }

    SECTION("more crossings means more waypoints per stroke") {
        ZigZag sparse { core_one_pad, 1.0f, 0.0f };
        ZigZag dense { core_one_pad, 4.0f, 0.0f };
        CHECK(run_stroke(sparse, core_one_pad.along_far).size()
            < run_stroke(dense, core_one_pad.along_far).size());
    }
}

TEST_CASE("ZigZag yields nothing without a stroke") {
    ZigZag zigzag { core_one_pad, 1.5f, 0.0f };
    CHECK_FALSE(zigzag.next().has_value());

    // Draining a stroke leaves the zigzag idle again
    run_stroke(zigzag, core_one_pad.along_far);
    CHECK_FALSE(zigzag.next().has_value());
}

TEST_CASE("ZigZag begin_stroke discards an unfinished stroke") {
    ZigZag zigzag { core_one_pad, 4.0f, 0.0f };

    // Abandon a stroke to the middle of the pad, then restart to the far end. A
    // begin_stroke that kept the old stroke would end on the middle target.
    const float mid_pad = 181.5f;
    zigzag.begin_stroke(mid_pad);
    REQUIRE(zigzag.next().has_value());

    const auto restarted = run_stroke(zigzag, core_one_pad.along_far);
    REQUIRE_FALSE(restarted.empty());
    CHECK(restarted.back().along == Approx(core_one_pad.along_far));
}
