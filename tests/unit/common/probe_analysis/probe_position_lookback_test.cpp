#include "catch2/catch.hpp"
#include "probe_position_lookback.hpp"
#include "math.h"
using buddy::ProbePositionLookback;

uint32_t ticks = 0;

extern "C" uint32_t ticks_us() {
    return ticks;
}

TEST_CASE("probe_position_lookback_basic") {
    ProbePositionLookback l;

    l.add_sample(1000, 1);
    l.add_sample(2000, 20);
    l.add_sample(3000, 30);
    l.add_sample(4000, 20);

    ticks = 5000;

    REQUIRE(1 == l.get_position_at(1000, []() { return 35; }));
    REQUIRE(2.9f == l.get_position_at(1100, []() { return 35; }));
    REQUIRE(10.5 == l.get_position_at(1500, []() { return 35; }));
    REQUIRE(20 == l.get_position_at(2000, []() { return 35; }));
    REQUIRE(25 == l.get_position_at(2500, []() { return 35; }));
    REQUIRE(30 == l.get_position_at(3000, []() { return 35; }));
    REQUIRE(25 == l.get_position_at(3500, []() { return 35; }));
    REQUIRE(20 == l.get_position_at(4000, []() { return 35; }));
    REQUIRE(27.5f == l.get_position_at(4500, []() { return 35; }));
    REQUIRE(35 == l.get_position_at(5000, []() { return 35; }));
}

TEST_CASE("probe_position_lookback_buffer_wraparound") {
    ProbePositionLookback l;

    // add many samples
    for (uint32_t i = 0; i < 1000; i++) {
        l.add_sample(i, i);

        ticks = i + 1;

        // check that we can always get last 16 samples back correctly
        for (uint32_t j = 0; j < 16; j++) {
            if (j > i) {
                break;
            }
            uint32_t val = i - j;
            REQUIRE((float)val == l.get_position_at(val, []() { return 999; }));
        }
        // and check that current sample is also correct
        REQUIRE(999.0f == l.get_position_at(ticks, []() { return 999.0f; }));

        // and check that 17th sample is not there anymore
        REQUIRE(isnan(l.get_position_at(i - 17, []() { return 999; })));
    }
}

TEST_CASE("probe_position_lookback_timer_wraparound") {
    ProbePositionLookback l;

    constexpr uint32_t max = std::numeric_limits<uint32_t>::max();
    constexpr uint32_t step = 1000;
    constexpr uint32_t start = max - step / 2;
    constexpr uint32_t stop = max + step / 2;

    l.add_sample(start - step, 100);
    l.add_sample(start, 200);
    l.add_sample(stop, 300);
    l.add_sample(stop + step, 400);
    ticks = stop + step * 2;

    REQUIRE(100 == l.get_position_at(start - step, []() { return 500; }));
    REQUIRE(200 == l.get_position_at(start, []() { return 500; }));
    REQUIRE(210 == l.get_position_at(start + 100, []() { return 500; }));
    REQUIRE(210 == l.get_position_at(start + 100, []() { return 500; }));
    REQUIRE(250 == l.get_position_at(start + 500, []() { return 500; }));
    REQUIRE(290 == l.get_position_at(start + 900, []() { return 500; }));
    REQUIRE(300 == l.get_position_at(stop, []() { return 500; }));
    REQUIRE(400 == l.get_position_at(stop + step, []() { return 500; }));
    REQUIRE(500 == l.get_position_at(stop + step * 2, []() { return 500; }));
}

TEST_CASE("probe_position_nan_return_test") {
    // test case that was actually failing in XL, problem was time difference was too high
    ProbePositionLookback l;

    l.add_sample(32569660, 0);
    l.add_sample(32571686, 0);
    l.add_sample(32573714, 0);
    l.add_sample(32575738, 0);
    l.add_sample(32577764, 0);
    l.add_sample(32579790, 0);
    l.add_sample(32581815, 0);
    l.add_sample(32583842, 0);
    l.add_sample(32585867, 0);
    l.add_sample(32555480, 0);
    l.add_sample(32557505, 0);
    l.add_sample(32559551, 0);
    l.add_sample(32561557, 0);
    l.add_sample(32563583, 0);
    l.add_sample(32565610, 0);
    l.add_sample(32567635, 0);

    ticks = 32580874;

    REQUIRE(0 == l.get_position_at(32580874, []() { return 0; }));
}
