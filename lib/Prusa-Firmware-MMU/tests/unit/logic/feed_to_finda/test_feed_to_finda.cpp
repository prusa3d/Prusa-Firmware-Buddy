#include "catch2/catch_test_macros.hpp"

#include "../../../../src/modules/buttons.h"
#include "../../../../src/modules/finda.h"
#include "../../../../src/modules/fsensor.h"
#include "../../../../src/modules/globals.h"
#include "../../../../src/modules/idler.h"
#include "../../../../src/modules/leds.h"
#include "../../../../src/modules/motion.h"
#include "../../../../src/modules/permanent_storage.h"
#include "../../../../src/modules/selector.h"

#include "../../../../src/logic/feed_to_finda.h"

#include "../../modules/stubs/stub_adc.h"

#include "../stubs/main_loop_stub.h"
#include "../stubs/stub_motion.h"

namespace ha = hal::adc;

TEST_CASE("feed_to_finda::feed_phase_unlimited", "[feed_to_finda]") {
    using namespace logic;

    ForceReinitAllAutomata();
    REQUIRE(EnsureActiveSlotIndex(0, mg::FilamentLoadState::AtPulley));

    FeedToFinda ff;
    main_loop();

    // restart the automaton
    REQUIRE(ff.Reset(false, true));

    REQUIRE(ff.State() == FeedToFinda::EngagingIdler);

    // it should have instructed the selector and idler to move to slot 1
    // check if the idler and selector have the right command
    CHECK(mm::AxisNearestTargetPos(mm::Idler) == mi::Idler::SlotPosition(0).v);
    CHECK(mm::AxisNearestTargetPos(mm::Selector) == ms::Selector::SlotPosition(0).v);

    // engaging idler
    REQUIRE(WhileCondition(
        ff,
        [&](uint32_t) { return !mi::idler.Engaged(); },
        5000));

    CHECK(mm::axes[mm::Idler].pos == mi::Idler::SlotPosition(0).v);
    CHECK(mm::axes[mm::Selector].pos == ms::Selector::SlotPosition(0).v);
    CHECK(mm::axes[mm::Pulley].enabled == true);

    // idler engaged, selector in position, we'll start pushing filament
    REQUIRE(ff.State() == FeedToFinda::PushingFilamentUnlimited);
    // at least at the beginning the LED should shine green (it should be blinking, but this mode has been already verified in the LED's unit test)
    REQUIRE(ml::leds.Mode(mg::globals.ActiveSlot(), ml::green) == ml::blink0);

    // now let the filament be pushed into the FINDA - do 500 steps without triggering the condition
    hal::gpio::WritePin(FINDA_PIN, hal::gpio::Level::high);

    REQUIRE(WhileCondition(
        ff,
        [&](uint32_t) { return ff.State() == FeedToFinda::PushingFilamentUnlimited; },
        1500));
    // From now on the FINDA is reported as ON

    //    // unloading back to PTFE
    //    REQUIRE(ff.State() == FeedToFinda::UnloadBackToPTFE);
    //    REQUIRE(WhileCondition(
    //        ff,
    //        [&](int) { return ff.State() == FeedToFinda::UnloadBackToPTFE; },
    //        5000));

    //    // disengaging idler
    //    REQUIRE(ff.State() == FeedToFinda::DisengagingIdler);
    //    REQUIRE(WhileCondition(
    //        ff,
    //        [&](int) { return mi::idler.Engaged(); },
    //        5000));

    //    CHECK(mm::axes[mm::Idler].pos == mi::Idler::SlotPosition(5).v); // @@TODO constants
    CHECK(mm::axes[mm::Selector].pos == ms::Selector::SlotPosition(0).v);
    CHECK(mm::axes[mm::Pulley].enabled == true);

    // state machine finished ok, the green LED should be blinking
    REQUIRE(ff.State() == FeedToFinda::OK);
    REQUIRE(ml::leds.Mode(mg::globals.ActiveSlot(), ml::green) == ml::blink0);

    REQUIRE(ff.Step() == true); // the automaton finished its work, any consecutive calls to Step must return true
}

TEST_CASE("feed_to_finda::FINDA_failed", "[feed_to_finda]") {
    using namespace logic;

    ForceReinitAllAutomata();
    REQUIRE(EnsureActiveSlotIndex(0, mg::FilamentLoadState::AtPulley));

    FeedToFinda ff;
    main_loop();

    // restart the automaton - we want the limited version of the feed
    REQUIRE(ff.Reset(true, true));

    REQUIRE(ff.State() == FeedToFinda::EngagingIdler);

    // it should have instructed the selector and idler to move to slot 1
    // check if the idler and selector have the right command
    CHECK(mm::AxisNearestTargetPos(mm::Idler) == mi::Idler::SlotPosition(0).v);
    CHECK(mm::AxisNearestTargetPos(mm::Selector) == ms::Selector::SlotPosition(0).v);

    // engaging idler
    REQUIRE(WhileCondition(
        ff,
        [&](uint32_t) { return !mi::idler.Engaged(); },
        5000));

    CHECK(mm::axes[mm::Idler].pos == mi::Idler::SlotPosition(0).v);
    CHECK(mm::axes[mm::Selector].pos == ms::Selector::SlotPosition(0).v);

    // idler engaged, we'll start pushing filament
    REQUIRE(ff.State() == FeedToFinda::PushingFilament);
    // at least at the beginning the LED should shine green (it should be blinking, but this mode has been already verified in the LED's unit test)
    REQUIRE(ml::leds.Mode(mg::globals.ActiveSlot(), ml::green) == ml::blink0);

    // now let the filament be pushed into the FINDA - but we make sure the FINDA doesn't trigger at all
    hal::gpio::WritePin(FINDA_PIN, hal::gpio::Level::low);

    REQUIRE(WhileCondition(
        ff,
        [&](uint32_t) { return ff.State() == FeedToFinda::PushingFilament; },
        10000));

    // the FINDA didn't trigger, we should be in the Failed state
    REQUIRE(ff.State() == FeedToFinda::Failed);
    REQUIRE(ml::leds.Mode(mg::globals.ActiveSlot(), ml::green) == ml::off);
    REQUIRE(ml::leds.Mode(mg::globals.ActiveSlot(), ml::red) == ml::blink0);

    REQUIRE(ff.Step() == true); // the automaton finished its work, any consecutive calls to Step must return true
}
