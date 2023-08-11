#include <catch2/catch.hpp>
#include <cstdint>
#include <functional>

#include "puppies/time_sync.hpp"

#include "timing.h"

using namespace buddy::puppies;

static constexpr auto REQUEST_DURATION_US = 2000;
static constexpr auto ITERATIONS = 1000;
static constexpr auto STEP_US = 1000;

static uint32_t current_ticks_us = 0;

uint32_t ticks_us() {
    return current_ticks_us;
}

struct Times {
    uint32_t buddy_time_us;
    uint32_t puppy_time_us;
};

void run_simulation(TimeSync &time_sync, std::function<Times(uint32_t)> calc_times) {
    for (unsigned int i = 0; i < ITERATIONS; ++i) {
        const Times times = calc_times(i);
        RequestTiming timing = {
            .begin_us = times.buddy_time_us - REQUEST_DURATION_US / 2,
            .end_us = times.buddy_time_us + REQUEST_DURATION_US / 2,
        };
        current_ticks_us = times.buddy_time_us;
        time_sync.sync(times.puppy_time_us, timing);
    }
}

TEST_CASE("Simple time sync") {
    // No crazy overflows, just test o initial conditions, no time drift
    TimeSync time_sync(0);

    REQUIRE(!time_sync.is_time_sync_valid());

    run_simulation(time_sync, [](uint32_t i) {
        const uint32_t buddy_time_us = 10000 + STEP_US * i;
        return Times {
            .buddy_time_us = buddy_time_us,
            .puppy_time_us = buddy_time_us,
        };
    });

    REQUIRE(time_sync.is_time_sync_valid());
    REQUIRE(abs(static_cast<int>(time_sync.buddy_time_us(10000)) - 10000) < 10);
}

TEST_CASE("Offset compensation") {
    TimeSync time_sync(0);

    const auto OFFSET_US = 1000;

    run_simulation(time_sync, [](uint32_t i) {
        const uint32_t buddy_time_us = 10000 + STEP_US * i;
        return Times {
            .buddy_time_us = buddy_time_us,
            .puppy_time_us = buddy_time_us + OFFSET_US, // Puppy is running 1000 ahead
        };
    });

    REQUIRE(abs(static_cast<int>(time_sync.buddy_time_us(10000 + OFFSET_US)) - 10000) < 10);
}

TEST_CASE("Drift compensation") {
    TimeSync time_sync(0);

    run_simulation(time_sync, [](uint32_t i) {
        const uint32_t buddy_time_us = 10000 + STEP_US * i;
        return Times {
            .buddy_time_us = buddy_time_us,
            .puppy_time_us = 101 * buddy_time_us / 100, // Puppy is running 1% faster
        };
    });

    // Plain offset is inaccurate as average drift is slightly outdated
    uint32_t puppy_time_us = ticks_us() * 1.01;
    REQUIRE(abs(static_cast<int>(time_sync.buddy_time_us(puppy_time_us)) - static_cast<int>(ticks_us())) < 60);

    current_ticks_us += 10000;

    // Include drift compensation (time passed while no sync performed)
    puppy_time_us = ticks_us() * 1.01;
    REQUIRE(abs(static_cast<int>(time_sync.buddy_time_us(puppy_time_us)) - static_cast<int>(ticks_us())) < 60);
}

TEST_CASE("Both overflow") {
    TimeSync time_sync(0);

    run_simulation(time_sync, [](uint32_t i) {
        const uint32_t buddy_time_us = std::numeric_limits<uint32_t>::max() - (ITERATIONS / 2 * STEP_US) + STEP_US * i;
        return Times {
            .buddy_time_us = buddy_time_us,
            .puppy_time_us = buddy_time_us,
        };
    });

    // Plain offset is inaccurate as average drift is slightly outdated
    REQUIRE(time_sync.buddy_time_us(10000) == 10000);

    current_ticks_us += 10000;

    // Include drift compensation (time passed while no sync performed)
    REQUIRE(time_sync.buddy_time_us(10000) == 10000);
}

TEST_CASE("Both overflow with drift") {
    TimeSync time_sync(0);

    run_simulation(time_sync, [](uint32_t i) {
        const uint32_t buddy_time_us = std::numeric_limits<uint32_t>::max() - (ITERATIONS / 2 * STEP_US) + STEP_US * i;
        return Times {
            .buddy_time_us = buddy_time_us,
            .puppy_time_us = 101 * buddy_time_us / 100, // Puppy is running 1% faster
        };
    });

    // Plain offset is inaccurate as average drift is slightly outdated
    uint32_t puppy_time_us = ticks_us() * 1.01;
    REQUIRE(abs(static_cast<int>(time_sync.buddy_time_us(puppy_time_us)) - static_cast<int>(ticks_us())) < 60);

    current_ticks_us += 10000;

    // Include drift compensation (time passed while no sync performed)
    puppy_time_us = ticks_us() * 1.01;
    REQUIRE(abs(static_cast<int>(time_sync.buddy_time_us(puppy_time_us)) - static_cast<int>(ticks_us())) < 60);
}

TEST_CASE("Buddy overflow") {
    TimeSync time_sync(0);

    run_simulation(time_sync, [](uint32_t i) {
        return Times {
            .buddy_time_us = std::numeric_limits<uint32_t>::max() - (ITERATIONS / 2 * STEP_US) + STEP_US * i,
            .puppy_time_us = std::numeric_limits<uint32_t>::max() - (ITERATIONS * STEP_US) + STEP_US * i,
        };
    });

    REQUIRE(time_sync.buddy_time_us(abs((static_cast<int>(ticks_us()) - (ITERATIONS / 2 * STEP_US)) - static_cast<int>(ticks_us())) < 10));
}

TEST_CASE("Puppy overflow") {
    TimeSync time_sync(0);

    run_simulation(time_sync, [](uint32_t i) {
        return Times {
            .buddy_time_us = std::numeric_limits<uint32_t>::max() - (ITERATIONS * STEP_US) + STEP_US * i,
            .puppy_time_us = std::numeric_limits<uint32_t>::max() - (ITERATIONS / 2 * STEP_US) + STEP_US * i,
        };
    });

    REQUIRE(time_sync.buddy_time_us(abs((static_cast<int>(ticks_us()) + (ITERATIONS / 2 * STEP_US))) - static_cast<int>(ticks_us())) < 10);
}

TEST_CASE("Offset overflow") {
    TimeSync time_sync(0);

    run_simulation(time_sync, [](uint32_t i) {
        return Times {
            .buddy_time_us = std::numeric_limits<uint32_t>::max() - (ITERATIONS / 2 * STEP_US) + STEP_US * i,
            .puppy_time_us = 0 + STEP_US * i,
        };
    });

    REQUIRE(abs(static_cast<int>(time_sync.buddy_time_us(0)) - static_cast<int>((std::numeric_limits<uint32_t>::max() - (ITERATIONS / 2 * STEP_US)))) < 10);
}
