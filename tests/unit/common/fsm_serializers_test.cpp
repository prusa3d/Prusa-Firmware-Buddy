/**
 * @file fsm_serializers_test.cpp
 * @author Radek Vana
 * @brief Unit tests for fsm types, especially fsm::SmartQueue
 * @date 2021-02-22
 */

// #define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include "catch2/catch.hpp"

#include "fsm_types.hpp"
#include "fsm_loadunload_type.hpp"
#include "selftest_axis_type.hpp"
#include "selftest_fans_type.hpp"
#include "selftest_heaters_type.hpp"

using namespace fsm;

template <class OBJ, class... Types>
void serialize_deserialize(OBJ cl, Types... types) {
    auto serialized = cl.Serialize();

    OBJ ctor_tst(serialized);
    OBJ deserialize_tst(types...);
    deserialize_tst.Deserialize(serialized);

    REQUIRE(ctor_tst == cl);
    REQUIRE(deserialize_tst == cl);
}

static constexpr int repeat_count = 4; // must be int, RangeGenerator takes template parameters
template <class T>
using Array = std::array<T, repeat_count>;

/*****************************************************************************/
// tests
TEST_CASE("ProgressSerializerLoadUnload", "[fsm]") {
    uint8_t progress = GENERATE(0, 1, 5, 0xFF);

    ProgressSerializerLoadUnload cl(LoadUnloadMode::Load, progress);
    REQUIRE(cl.progress == progress);

    serialize_deserialize(cl, LoadUnloadMode::Load);

    SECTION("equality") {
        uint8_t progress2 = GENERATE(0, 1, 55, 0xFF);
        LoadUnloadMode mode = GENERATE(LoadUnloadMode::Load, LoadUnloadMode::Purge);
        ProgressSerializerLoadUnload cl2(mode, progress2);

        // progresses with equal data are equal
        REQUIRE((cl == cl2) == ((progress == progress2) && (mode == LoadUnloadMode::Load)));
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

    SelftestAxis_t cl(
        SelftestSingleAxis_t(x_progress[cycle], x_state[cycle]),
        SelftestSingleAxis_t(y_progress[cycle], y_state[cycle]),
        SelftestSingleAxis_t(z_progress[cycle], z_state[cycle]));

    REQUIRE(cl.x_progress == x_progress[cycle]);
    REQUIRE(cl.y_progress == y_progress[cycle]);
    REQUIRE(cl.z_progress == z_progress[cycle]);
    REQUIRE(cl.x_state == x_state[cycle]);
    REQUIRE(cl.y_state == y_state[cycle]);
    REQUIRE(cl.z_state == z_state[cycle]);

    serialize_deserialize(cl);

    SECTION("equality") {
        int cycle2 = GENERATE(range(0, repeat_count - 1));
        SelftestAxis_t cl2(
            SelftestSingleAxis_t(x_progress[cycle2], x_state[cycle2]),
            SelftestSingleAxis_t(y_progress[cycle2], y_state[cycle2]),
            SelftestSingleAxis_t(z_progress[cycle2], z_state[cycle2]));

        // progeses with equal data are equal
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

    SelftestHeaters_t cl;
    cl.noz[0].progress = noz_progress[cycle];
    cl.bed.progress = bed_progress[cycle];
    cl.noz[0].prep_state = noz_prep_state[cycle];
    cl.noz[0].heat_state = noz_heat_state[cycle];
    cl.bed.prep_state = bed_prep_state[cycle];
    cl.bed.heat_state = bed_heat_state[cycle];

    // SelftestHeaters_t is serialized as extended fsm data, so only via assignment operator
    SelftestHeaters_t copy = cl;
    cl = copy;
    REQUIRE(cl == copy);

    REQUIRE(cl.noz[0].progress == noz_progress[cycle]);
    REQUIRE(cl.bed.progress == bed_progress[cycle]);
    REQUIRE(cl.noz[0].prep_state == noz_prep_state[cycle]);
    REQUIRE(cl.noz[0].heat_state == noz_heat_state[cycle]);
    REQUIRE(cl.bed.prep_state == bed_prep_state[cycle]);
    REQUIRE(cl.bed.heat_state == bed_heat_state[cycle]);

    SECTION("equality") {
        int cycle2 = GENERATE(range(0, repeat_count - 1));
        SelftestHeaters_t cl2;
        cl2.noz[0].progress = noz_progress[cycle2];
        cl2.bed.progress = bed_progress[cycle2];
        cl2.noz[0].prep_state = noz_prep_state[cycle2];
        cl2.noz[0].heat_state = noz_heat_state[cycle2];
        cl2.bed.prep_state = bed_prep_state[cycle2];
        cl2.bed.heat_state = bed_heat_state[cycle2];

        // progeses with equal data are equal
        REQUIRE((cl == cl2) == (cycle == cycle2));
    }
}
