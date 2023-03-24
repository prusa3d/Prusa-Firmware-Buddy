#include "catch2/catch.hpp"

#include "../../../../src/modules/buttons.h"
#include "../../../../src/modules/finda.h"
#include "../../../../src/modules/fsensor.h"
#include "../../../../src/modules/globals.h"
#include "../../../../src/modules/idler.h"
#include "../../../../src/modules/leds.h"
#include "../../../../src/modules/motion.h"
#include "../../../../src/modules/permanent_storage.h"
#include "../../../../src/modules/selector.h"

#include "../../../../src/logic/unload_to_finda.h"

#include "../../modules/stubs/stub_adc.h"

#include "../stubs/main_loop_stub.h"
#include "../stubs/stub_motion.h"

using Catch::Matchers::Equals;

namespace ha = hal::adc;

TEST_CASE("unload_to_finda::regular_unload", "[unload_to_finda]") {
    ForceReinitAllAutomata();
    EnsureActiveSlotIndex(0, mg::FilamentLoadState::AtPulley);

    // we need finda ON
    SetFINDAStateAndDebounce(true);
    // and MMU "thinks" it has the filament loaded
    mg::globals.SetFilamentLoaded(mg::globals.ActiveSlot(), mg::FilamentLoadState::InNozzle);

    logic::UnloadToFinda ff;

    // restart the automaton - just 1 attempt
    ff.Reset(1);

    REQUIRE(ff.State() == logic::UnloadToFinda::EngagingIdler);

    // it should have instructed the selector and idler to move to slot 1
    // check if the idler and selector have the right command
    CHECK(mm::axes[mm::Idler].targetPos == mi::Idler::SlotPosition(0).v);
    CHECK(mm::axes[mm::Selector].targetPos == ms::Selector::SlotPosition(0).v);

    // engaging idler
    REQUIRE(WhileCondition(
        ff,
        [&](uint32_t) { return !mi::idler.Engaged(); },
        5000));
    CHECK(mm::axes[mm::Pulley].enabled == true);

    // now pulling the filament until finda triggers
    REQUIRE(ff.State() == logic::UnloadToFinda::WaitingForFINDA);
    hal::gpio::WritePin(FINDA_PIN, hal::gpio::Level::low);
    REQUIRE(WhileCondition(
        ff,
        [&](uint32_t) { return mf::finda.Pressed(); },
        50000));

    REQUIRE(ff.State() == logic::UnloadToFinda::OK);
    REQUIRE(mg::globals.FilamentLoaded() == mg::FilamentLoadState::InSelector);
}

TEST_CASE("unload_to_finda::no_sense_FINDA_upon_start", "[unload_to_finda]") {
    ForceReinitAllAutomata(); // that implies FINDA OFF which should really not happen for an unload call
    EnsureActiveSlotIndex(0, mg::FilamentLoadState::AtPulley);

    logic::UnloadToFinda ff;

    // restart the automaton - just 1 attempt
    ff.Reset(1);

    // the state machine should accept the unpressed FINDA as no-fillament-loaded
    // thus should immediately end in the OK state
    REQUIRE(ff.State() == logic::UnloadToFinda::OK);
}

TEST_CASE("unload_to_finda::unload_without_FINDA_trigger", "[unload_to_finda]") {
    ForceReinitAllAutomata();
    EnsureActiveSlotIndex(0, mg::FilamentLoadState::AtPulley);

    // we need finda ON
    SetFINDAStateAndDebounce(true);
    // and MMU "thinks" it has the filament loaded
    mg::globals.SetFilamentLoaded(mg::globals.ActiveSlot(), mg::FilamentLoadState::InNozzle);

    logic::UnloadToFinda ff;

    // restart the automaton - just 1 attempt
    ff.Reset(1);

    REQUIRE(ff.State() == logic::UnloadToFinda::EngagingIdler);

    // it should have instructed the selector and idler to move to slot 1
    // check if the idler and selector have the right command
    CHECK(mm::axes[mm::Idler].targetPos == mi::Idler::SlotPosition(0).v);
    CHECK(mm::axes[mm::Selector].targetPos == ms::Selector::SlotPosition(0).v);
    CHECK(mm::axes[mm::Idler].enabled == true);

    // engaging idler
    REQUIRE(WhileCondition(
        ff,
        [&](uint32_t) { return !mi::idler.Engaged(); },
        5000));

    // now pulling the filament until finda triggers
    REQUIRE(ff.State() == logic::UnloadToFinda::WaitingForFINDA);

    // no changes to FINDA during unload - we'll pretend it never triggers
    REQUIRE_FALSE(WhileCondition(
        ff,
        [&](uint32_t) { return mf::finda.Pressed(); },
        50000));

    REQUIRE(ff.State() == logic::UnloadToFinda::Failed);
    REQUIRE(mg::globals.FilamentLoaded() == mg::FilamentLoadState::InSelector);
}
