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

#include "../../../../src/logic/feed_to_bondtech.h"

#include "../../modules/stubs/stub_adc.h"

#include "../stubs/main_loop_stub.h"
#include "../stubs/stub_motion.h"

namespace ha = hal::adc;

TEST_CASE("feed_to_bondtech::feed_phase_unlimited", "[feed_to_bondtech]") {
    using namespace logic;

    uint8_t slot = 0;

    ForceReinitAllAutomata();
    REQUIRE(EnsureActiveSlotIndex(slot, mg::FilamentLoadState::AtPulley));

    // reset bowden lenghts in EEPROM
    SetMinimalBowdenLength();
    // check bowden lengths
    REQUIRE(mps::BowdenLength::Get() == config::minimumBowdenLength.v);

    FeedToBondtech fb;
    main_loop();

    // restart the automaton
    fb.Reset(1);

    REQUIRE(fb.State() == FeedToBondtech::EngagingIdler);

    // it should have instructed the selector and idler to move to a slot
    // check if the idler and selector have the right command
    CHECK(mm::AxisNearestTargetPos(mm::Idler) == mi::Idler::SlotPosition(slot).v);
    CHECK(mm::AxisNearestTargetPos(mm::Selector) == ms::Selector::SlotPosition(slot).v);
    CHECK(mm::axes[mm::Idler].enabled == true);

    // engaging idler
    REQUIRE(WhileCondition(
        fb,
        [&](uint32_t) { return !mi::idler.Engaged(); },
        5000));

    CHECK(mm::axes[mm::Idler].pos == mi::Idler::SlotPosition(slot).v);
    CHECK(mm::axes[mm::Selector].pos == ms::Selector::SlotPosition(slot).v);
    CHECK(mm::axes[mm::Pulley].enabled);

    // idler engaged, selector in position, we'll start pushing filament
    REQUIRE(fb.State() == FeedToBondtech::PushingFilamentFast);
    // at least at the beginning the LED should shine green (it should be blinking, but this mode has been already verified in the LED's unit test)
    REQUIRE(ml::leds.Mode(mg::globals.ActiveSlot(), ml::green) == ml::blink0);

    // fast load - no fsensor trigger
    // performs fast load for config::minimumBowdenLength distance
    REQUIRE(WhileCondition(
        fb,
        [&](uint32_t) { return fb.State() == FeedToBondtech::PushingFilamentFast; },
        mm::unitToSteps<mm::P_pos_t>(config::minimumBowdenLength) + 2));

    // slow load - expecting fsensor trigger
    // This gets interesting with bowden length autotuning - we should trigger at the right step
    constexpr uint32_t additionalBowdenLengthTrigger = mm::unitToSteps<mm::P_pos_t>(config::defaultBowdenLength - config::minimumBowdenLength);
    REQUIRE(WhileCondition(
        fb,
        [&](uint32_t step) {
        if( step == additionalBowdenLengthTrigger ){
            mfs::fsensor.ProcessMessage(true);
        }
        return fb.State() == FeedToBondtech::PushingFilamentToFSensor; },
        additionalBowdenLengthTrigger + 5));

    REQUIRE(mfs::fsensor.Pressed());

    // pushing filament from fsensor into the nozzle
    REQUIRE(fb.State() == FeedToBondtech::PushingFilamentIntoNozzle);
    REQUIRE(WhileCondition(
        fb,
        [&](uint32_t) { return fb.State() == FeedToBondtech::PushingFilamentIntoNozzle; },
        5000));

    // partially disengaging idler
    REQUIRE(fb.State() == FeedToBondtech::PartiallyDisengagingIdler);
    REQUIRE(WhileCondition(
        fb,
        [&](uint32_t) { return fb.State() == FeedToBondtech::PartiallyDisengagingIdler; },
        5000));

    CHECK(mm::axes[mm::Idler].pos == mi::Idler::IntermediateSlotPosition(slot).v);
    CHECK(mm::axes[mm::Selector].pos == ms::Selector::SlotPosition(slot).v);
    CHECK_FALSE(mm::axes[mm::Pulley].enabled);

    // fully disengaging idler
    REQUIRE(fb.State() == FeedToBondtech::DisengagingIdler);
    REQUIRE(WhileCondition(
        fb,
        [&](uint32_t) { return fb.State() == FeedToBondtech::DisengagingIdler; },
        5000));

    CHECK(mm::axes[mm::Idler].pos == mi::Idler::SlotPosition(mi::idler.IdleSlotIndex()).v);
    CHECK(mm::axes[mm::Selector].pos == ms::Selector::SlotPosition(slot).v);
    CHECK_FALSE(mm::axes[mm::Pulley].enabled);

    // state machine finished ok, the green LED should be on
    REQUIRE(fb.State() == FeedToBondtech::OK);
    REQUIRE(ml::leds.LedOn(mg::globals.ActiveSlot(), ml::green));

    // detected bowden length is expected still to be 341, not runtime detection available
    uint16_t bowdenLength = mps::BowdenLength::Get();
    CHECK(bowdenLength == 341);
    // must be within the specified tolerance of 10mm from the default bowden length
    REQUIRE(abs(bowdenLength - config::minimumBowdenLength.v) < 10);

    REQUIRE(fb.Step() == true); // the automaton finished its work, any consecutive calls to Step must return true
}
