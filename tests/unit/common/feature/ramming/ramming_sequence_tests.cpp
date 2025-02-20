#include <feature/ramming/ramming_sequence.hpp>

#include <catch2/catch.hpp>

using namespace buddy;

TEST_CASE("RammingSequence") {
    static constexpr RammingSequenceArray seq({
        { 8, 995 },
        { -43, 6000 },
        { -8, 3000 },
        { -4, 1800 },
        { 20, 600 },
        { -20, 470 },
        { 55, 1740 },
        { -55, 6000 },
        { 20, 340 },
        { -20, 210 },
    });
    CHECK(seq.retracted_distance() == 55);
}
