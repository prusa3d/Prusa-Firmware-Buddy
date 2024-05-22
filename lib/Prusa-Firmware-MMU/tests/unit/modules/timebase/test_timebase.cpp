#include "catch2/catch_test_macros.hpp"
#include "timebase.h"
#include "../stubs/stub_timebase.h"

// this is not a pure test of the real implementation (it would require splitting the timebase.cpp into 2 parts)
// but serves the sole purpose of debugging the Elapsed() impl.
TEST_CASE("timebase::Elapsed", "[timebase]") {
    {
        mt::ReinitTimebase(0);
        uint16_t start = mt::timebase.Millis();
        mt::IncMillis(5);
        REQUIRE(mt::timebase.Elapsed(start, 4));
    }
    {
        mt::ReinitTimebase(0xffff);
        uint16_t start = mt::timebase.Millis();
        mt::IncMillis(5);
        REQUIRE(mt::timebase.Elapsed(start, 4));
    }
}
