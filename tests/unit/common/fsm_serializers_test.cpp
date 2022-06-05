/**
 * @file fsm_serializers_test.cpp
 * @author Radek Vana
 * @brief Unit tests for fsm types, especially fsm::SmartQueue
 * @date 2021-02-22
 */

//#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include "catch2/catch.hpp"

#include "fsm_types.hpp"
#include "fsm_progress_type.hpp"
#include "selftest_axis_type.hpp"
#include "selftest_fans_type.hpp"
#include "selftest_heaters_type.hpp"

using namespace fsm;

template <class OBJ>
void serialize_deserialize(OBJ cl) {
    auto serialized = cl.Serialize();

    OBJ ctor_tst(serialized);
    OBJ deserialize_tst;
    deserialize_tst.Deserialize(serialized);

    REQUIRE(ctor_tst == cl);
    REQUIRE(deserialize_tst == cl);
}

static constexpr int repeat_count = 4; // must be int, RangeGenerator takes template parameters
template <class T>
using Array = std::array<T, repeat_count>;

/*****************************************************************************/
//tests
TEST_CASE("ProgressSerializer", "[fsm]") {
    uint8_t progress = GENERATE(0, 1, 5, 0xFF);

    ProgressSerializer cl(progress);
    REQUIRE(cl.progress == progress);

    serialize_deserialize(cl);

    SECTION("equality") {
        uint8_t progress2 = GENERATE(0, 1, 55, 0xFF);
        ProgressSerializer cl2(progress2);

        //progeses with equal data are equal
        REQUIRE((cl == cl2) == (progress == progress2));
    }
}
