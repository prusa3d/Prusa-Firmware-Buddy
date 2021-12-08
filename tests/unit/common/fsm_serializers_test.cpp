/**
 * @file fsm_serializers_test.cpp
 * @author Radek Vana
 * @brief Unit tests for fsm types, especially fsm::Queue
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

TEST_CASE("SelftestAxis_t", "[fsm]") {

    int cycle = GENERATE(range(0, repeat_count - 1));

    Array<uint8_t> x_progress = { { 0, 100, 22, 25 } };
    Array<uint8_t> y_progress = { { 0, 100, 33, 35 } };
    Array<uint8_t> z_progress = { { 0, 100, 11, 66 } };
    Array<SelftestSubtestState_t> x_state = { { SelftestSubtestState_t::undef, SelftestSubtestState_t::ok, SelftestSubtestState_t::not_good, SelftestSubtestState_t::running } };
    Array<SelftestSubtestState_t> y_state = { { SelftestSubtestState_t::undef, SelftestSubtestState_t::not_good, SelftestSubtestState_t::ok, SelftestSubtestState_t::running } };
    Array<SelftestSubtestState_t> z_state = { { SelftestSubtestState_t::undef, SelftestSubtestState_t::undef, SelftestSubtestState_t::running, SelftestSubtestState_t::running } };

    SelftestAxis_t cl(x_progress[cycle], y_progress[cycle], z_progress[cycle],
        x_state[cycle], y_state[cycle], z_state[cycle]);

    REQUIRE(cl.x_progress == x_progress[cycle]);
    REQUIRE(cl.y_progress == y_progress[cycle]);
    REQUIRE(cl.z_progress == z_progress[cycle]);
    REQUIRE(cl.x_state == x_state[cycle]);
    REQUIRE(cl.y_state == y_state[cycle]);
    REQUIRE(cl.z_state == z_state[cycle]);

    serialize_deserialize(cl);

    SECTION("equality") {
        int cycle2 = GENERATE(range(0, repeat_count - 1));
        SelftestAxis_t cl2(x_progress[cycle2], y_progress[cycle2], z_progress[cycle2],
            x_state[cycle2], y_state[cycle2], z_state[cycle2]);

        //progeses with equal data are equal
        REQUIRE((cl == cl2) == (cycle == cycle2));
    }
}

TEST_CASE("SelftestFans_t", "[fsm]") {

    int cycle = GENERATE(range(0, repeat_count - 1));

    Array<uint8_t> print_fan_progress = { { 0, 100, 22, 25 } };
    Array<uint8_t> nozzle_fan_progress = { { 0, 100, 33, 35 } };
    Array<uint8_t> tot_progress = { { 0, 100, 11, 66 } };
    Array<SelftestSubtestState_t> print_fan_state = { { SelftestSubtestState_t::undef, SelftestSubtestState_t::ok, SelftestSubtestState_t::not_good, SelftestSubtestState_t::running } };
    Array<SelftestSubtestState_t> nozzle_fan_state = { { SelftestSubtestState_t::undef, SelftestSubtestState_t::not_good, SelftestSubtestState_t::ok, SelftestSubtestState_t::running } };

    SelftestFans_t cl(print_fan_progress[cycle], nozzle_fan_progress[cycle], tot_progress[cycle],
        print_fan_state[cycle], nozzle_fan_state[cycle]);

    REQUIRE(cl.print_fan_progress == print_fan_progress[cycle]);
    REQUIRE(cl.heatbreak_fan_progress == nozzle_fan_progress[cycle]);
    REQUIRE(cl.tot_progress == tot_progress[cycle]);
    REQUIRE(cl.print_fan_state == print_fan_state[cycle]);
    REQUIRE(cl.heatbreak_fan_state == nozzle_fan_state[cycle]);

    serialize_deserialize(cl);

    SECTION("equality") {
        int cycle2 = GENERATE(range(0, repeat_count - 1));
        SelftestFans_t cl2(print_fan_progress[cycle2], nozzle_fan_progress[cycle2], tot_progress[cycle2],
            print_fan_state[cycle2], nozzle_fan_state[cycle2]);

        //progeses with equal data are equal
        REQUIRE((cl == cl2) == (cycle == cycle2));
    }
}

TEST_CASE("SelftestHeaters_t", "[fsm]") {

    int cycle = GENERATE(range(0, repeat_count - 1));

    Array<uint8_t> noz_progress = { { 0, 100, 22, 25 } };
    Array<uint8_t> bed_progress = { { 0, 100, 33, 35 } };
    Array<SelftestSubtestState_t> noz_prep_state = { { SelftestSubtestState_t::undef, SelftestSubtestState_t::ok, SelftestSubtestState_t::not_good, SelftestSubtestState_t::running } };
    Array<SelftestSubtestState_t> noz_heat_state = { { SelftestSubtestState_t::undef, SelftestSubtestState_t::not_good, SelftestSubtestState_t::ok, SelftestSubtestState_t::running } };
    Array<SelftestSubtestState_t> bed_prep_state = { { SelftestSubtestState_t::undef, SelftestSubtestState_t::undef, SelftestSubtestState_t::running, SelftestSubtestState_t::running } };
    Array<SelftestSubtestState_t> bed_heat_state = { { SelftestSubtestState_t::undef, SelftestSubtestState_t::running, SelftestSubtestState_t::undef, SelftestSubtestState_t::running } };

    SelftestHeaters_t cl(noz_progress[cycle], bed_progress[cycle], noz_prep_state[cycle],
        noz_heat_state[cycle], bed_prep_state[cycle], bed_heat_state[cycle]);

    REQUIRE(cl.noz_progress == noz_progress[cycle]);
    REQUIRE(cl.bed_progress == bed_progress[cycle]);
    REQUIRE(cl.noz_prep_state == noz_prep_state[cycle]);
    REQUIRE(cl.noz_heat_state == noz_heat_state[cycle]);
    REQUIRE(cl.bed_prep_state == bed_prep_state[cycle]);
    REQUIRE(cl.bed_heat_state == bed_heat_state[cycle]);

    serialize_deserialize(cl);

    SECTION("equality") {
        int cycle2 = GENERATE(range(0, repeat_count - 1));
        SelftestHeaters_t cl2(noz_progress[cycle2], bed_progress[cycle2], noz_prep_state[cycle2],
            noz_heat_state[cycle2], bed_prep_state[cycle2], bed_heat_state[cycle2]);

        //progeses with equal data are equal
        REQUIRE((cl == cl2) == (cycle == cycle2));
    }
}
