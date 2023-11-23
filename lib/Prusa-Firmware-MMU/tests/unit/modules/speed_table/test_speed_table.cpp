#include "catch2/catch_test_macros.hpp"
#include "speed_table.h"
#include <stdio.h>

using namespace modules::speed_table;

// The following reference values are calculated for 2MHz timer
static_assert(F_CPU / config::stepTimerFrequencyDivider == 2000000,
    "speed tables not compatible for the requested frequency");

static const st_timer_t reference[][3] = {
    { 1, 62500, 1 },
    { 501, 3992, 1 },
    { 1001, 1998, 1 },
    { 1501, 1332, 1 },
    { 2001, 1000, 1 },
    { 2501, 801, 1 },
    { 3001, 668, 1 },
    { 3501, 572, 1 },
    { 4001, 500, 1 },
    { 4501, 444, 1 },
    { 5001, 400, 1 },
    { 5501, 364, 1 },
    { 6001, 333, 1 },
    { 6501, 307, 1 },
    { 7001, 285, 1 },
    { 7501, 266, 1 },
    { 8001, 250, 1 },
    { 8501, 234, 1 },
    { 9001, 222, 1 },
    { 9501, 211, 1 },
    { 10001, 400, 2 },
    { 10501, 381, 2 },
    { 11001, 364, 2 },
    { 11501, 348, 2 },
    { 12001, 333, 2 },
    { 12501, 320, 2 },
    { 13001, 308, 2 },
    { 13501, 297, 2 },
    { 14001, 286, 2 },
    { 14501, 276, 2 },
    { 15001, 267, 2 },
    { 15501, 258, 2 },
    { 16001, 250, 2 },
    { 16501, 243, 2 },
    { 17001, 235, 2 },
    { 17501, 228, 2 },
    { 18001, 222, 2 },
    { 18501, 216, 2 },
    { 19001, 211, 2 },
    { 19501, 205, 2 },
    { 20001, 400, 4 },
    { 20501, 391, 4 },
    { 21001, 381, 4 },
    { 21501, 371, 4 },
    { 22001, 364, 4 },
    { 22501, 356, 4 },
    { 23001, 348, 4 },
    { 23501, 340, 4 },
    { 24001, 333, 4 },
    { 24501, 326, 4 },
    { 25001, 320, 4 },
    { 25501, 312, 4 },
    { 26001, 308, 4 },
    { 26501, 301, 4 },
    { 27001, 297, 4 },
    { 27501, 290, 4 },
    { 28001, 286, 4 },
    { 28501, 280, 4 },
    { 29001, 276, 4 },
    { 29501, 270, 4 },
    { 30001, 267, 4 },
    { 30501, 262, 4 },
    { 31001, 258, 4 },
    { 31501, 254, 4 },
    { 32001, 250, 4 },
    { 32501, 247, 4 },
    { 33001, 243, 4 },
    { 33501, 239, 4 },
    { 34001, 235, 4 },
    { 34501, 231, 4 },
    { 35001, 228, 4 },
    { 35501, 225, 4 },
    { 36001, 222, 4 },
    { 36501, 219, 4 },
    { 37001, 216, 4 },
    { 37501, 214, 4 },
    { 38001, 211, 4 },
    { 38501, 208, 4 },
    { 39001, 205, 4 },
    { 39501, 201, 4 },
};

TEST_CASE("speed_table::calc_timer", "[speed_table]") {
    // Check the result values of calc_timer against an AVR reference table
    for (unsigned i = 0; i != sizeof(reference) / sizeof(*reference); ++i) {
        st_timer_t step_rate = reference[i][0];
        uint8_t loops;
        st_timer_t timer = calc_timer(step_rate, loops);

        // allow +/-1 of difference for rounding between the C and ASM versions
        REQUIRE(abs((int)timer - (int)reference[i][1]) <= 1);

        // loops should be exact
        REQUIRE(loops == reference[i][2]);

        // show the table
        printf("%u %u %u\n", step_rate, timer, loops);
    }
}
