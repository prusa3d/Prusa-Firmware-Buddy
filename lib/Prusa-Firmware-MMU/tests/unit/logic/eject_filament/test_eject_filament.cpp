#include <functional>
#include "catch2/catch_test_macros.hpp"
#include "catch2/generators/catch_generators.hpp"

#include "../../../../src/modules/buttons.h"
#include "../../../../src/modules/finda.h"
#include "../../../../src/modules/fsensor.h"
#include "../../../../src/modules/globals.h"
#include "../../../../src/modules/idler.h"
#include "../../../../src/modules/leds.h"
#include "../../../../src/modules/motion.h"
#include "../../../../src/modules/permanent_storage.h"
#include "../../../../src/modules/selector.h"
#include "../../../../src/modules/user_input.h"

#include "../../../../src/logic/eject_filament.h"

#include "../../modules/stubs/stub_adc.h"
#include "../../modules/stubs/stub_timebase.h"

#include "../stubs/main_loop_stub.h"
#include "../stubs/stub_motion.h"

#include "../helpers/helpers.ipp"

TEST_CASE("eject_filament::eject0-4", "[eject_filament]") {
    using namespace logic;

    uint8_t ejectSlot = GENERATE(0, 1, 2, 3, 4);
    uint8_t selectorParkedPos = (ejectSlot <= 2) ? 4 : 0;
    INFO("Testing ejecting slot " << (int)ejectSlot);

    ForceReinitAllAutomata();

    REQUIRE(EnsureActiveSlotIndex(ejectSlot, mg::FilamentLoadState::AtPulley));

    EjectFilament ef;
    // restart the automaton
    ef.Reset(ejectSlot);

    main_loop();

    // Start at UnloadingFilament
    REQUIRE(ef.TopLevelState() == ProgressCode::UnloadingFilament);

    REQUIRE(WhileTopState(ef, ProgressCode::UnloadingFilament, 5000));

    REQUIRE(ef.TopLevelState() == ProgressCode::ParkingSelector);

    REQUIRE(WhileTopState(ef, ProgressCode::ParkingSelector, selectorMoveMaxSteps));

    // Engaging idler
    REQUIRE(ef.TopLevelState() == ProgressCode::EngagingIdler);

    REQUIRE(WhileTopState(ef, ProgressCode::EngagingIdler, 5000));

    REQUIRE(mi::idler.Engaged());
    REQUIRE(ef.TopLevelState() == ProgressCode::EjectingFilament);

    REQUIRE(WhileTopState(ef, ProgressCode::EjectingFilament, 5000));

    // should end up in error disengage idler
    REQUIRE(VerifyState2(ef, mg::FilamentLoadState::AtPulley, ejectSlot, selectorParkedPos, false, true, ejectSlot, ml::off, ml::blink0, ErrorCode::RUNNING, ProgressCode::ERRDisengagingIdler));

    SimulateErrDisengagingIdler(ef, ErrorCode::FILAMENT_EJECTED);

    // Pulley should now be disabled
    REQUIRE(VerifyState2(ef, mg::FilamentLoadState::AtPulley, mi::idler.IdleSlotIndex(), selectorParkedPos, false, false, ejectSlot, ml::off, ml::blink0, ErrorCode::FILAMENT_EJECTED, ProgressCode::ERRWaitingForUser));

    // Now press Done button
    PressButtonAndDebounce(ef, mb::Middle, true);
    ClearButtons(ef);

    // Idler and Selector are not on HOLD state
    REQUIRE(mi::idler.State() != mm::MovableBase::OnHold);
    REQUIRE(ms::selector.State() != mm::MovableBase::OnHold);

    // Error code is now OK
    // LEDs turn off at the ejected slot
    REQUIRE(VerifyState2(ef, mg::FilamentLoadState::AtPulley, mi::idler.IdleSlotIndex(), selectorParkedPos, false, false, ejectSlot, ml::off, ml::off, ErrorCode::OK, ProgressCode::OK));
}

TEST_CASE("eject_filament::invalid_slot", "[eject_filament]") {
    for (uint8_t activeSlot = 0; activeSlot < config::toolCount; ++activeSlot) {
        logic::EjectFilament ef;
        InvalidSlot<logic::EjectFilament>(ef, activeSlot, config::toolCount);
    }
}

TEST_CASE("eject_filament::flickering_FINDA", "[eject_filament]") {
    using namespace logic;
    using namespace std::placeholders;

    uint8_t ejectSlot = GENERATE(0, 1, 2, 3, 4);
    uint8_t selectorParkedPos = (ejectSlot <= 2) ? 4 : 0;
    INFO("Testing ejecting slot " << (int)ejectSlot);

    ForceReinitAllAutomata();

    REQUIRE(EnsureActiveSlotIndex(ejectSlot, mg::FilamentLoadState::AtPulley));

    EjectFilament ef;
    // restart the automaton
    ef.Reset(ejectSlot);

    main_loop();

    // Start at UnloadingFilament
    REQUIRE(ef.TopLevelState() == ProgressCode::UnloadingFilament);

    // This is something else than WhileTopState()==UnloadingFilament
    // We need to catch the very moment, when the unload finished and a move to another slot is being planned
    REQUIRE(WhileCondition(
        ef, [&](uint32_t) -> bool { return ef.unl.State() != ProgressCode::OK; }, 5000));

    // now press FINDA again, but prevent stepping other state machines
    REQUIRE_FALSE(mf::finda.Pressed());
    hal::gpio::WritePin(FINDA_PIN, hal::gpio::Level::high);
    while (!mf::finda.Pressed()) {
        mf::finda.Step();
        mt::IncMillis();
    }
    REQUIRE(mf::finda.Pressed());
    REQUIRE(mg::globals.FilamentLoaded() == mg::FilamentLoadState::AtPulley);

    main_loop();
    ef.Step();

    // Idler disengages, and error is pending
    REQUIRE(VerifyState2(ef, mg::FilamentLoadState::AtPulley, mi::idler.IdleSlotIndex(), ejectSlot, true, false, ejectSlot, ml::off, ml::blink0, ErrorCode::RUNNING, ProgressCode::ERRDisengagingIdler));

    SimulateErrDisengagingIdler(ef, ErrorCode::FINDA_FLICKERS); // this should be a single step, Idler should remain disengaged due to previous error

    // Validate waiting for user state
    REQUIRE_FALSE(mui::userInput.AnyEvent());
    REQUIRE(VerifyState2(ef, mg::FilamentLoadState::AtPulley, mi::idler.IdleSlotIndex(), ejectSlot, true, false, ejectSlot, ml::off, ml::blink0, ErrorCode::FINDA_FLICKERS, ProgressCode::ERRWaitingForUser));

    // Reset FINDA
    SetFINDAStateAndDebounce(false);

    // Press middle button to 'retry'
    PressButtonAndDebounce(ef, mb::Middle, true);
    ClearButtons(ef);

    // Now everything should continue as normal
    REQUIRE(VerifyState2(ef, mg::FilamentLoadState::AtPulley, mi::idler.IdleSlotIndex(), ejectSlot, false, false, ejectSlot, ml::off, ml::blink0, ErrorCode::RUNNING, ProgressCode::ParkingSelector));

    REQUIRE(WhileTopState(ef, ProgressCode::ParkingSelector, selectorMoveMaxSteps));

    // Engaging idler
    REQUIRE(ef.TopLevelState() == ProgressCode::EngagingIdler);

    REQUIRE(WhileTopState(ef, ProgressCode::EngagingIdler, 5000));

    REQUIRE(mi::idler.Engaged());
    REQUIRE(ef.TopLevelState() == ProgressCode::EjectingFilament);

    REQUIRE(WhileTopState(ef, ProgressCode::EjectingFilament, 5000));

    // should end up in error disengage idler
    REQUIRE(VerifyState2(ef, mg::FilamentLoadState::AtPulley, ejectSlot, selectorParkedPos, false, true, ejectSlot, ml::off, ml::blink0, ErrorCode::RUNNING, ProgressCode::ERRDisengagingIdler));

    SimulateErrDisengagingIdler(ef, ErrorCode::FILAMENT_EJECTED);

    // Pulley should now be disabled
    REQUIRE(VerifyState2(ef, mg::FilamentLoadState::AtPulley, mi::idler.IdleSlotIndex(), selectorParkedPos, false, false, ejectSlot, ml::off, ml::blink0, ErrorCode::FILAMENT_EJECTED, ProgressCode::ERRWaitingForUser));

    // Now press Done button
    PressButtonAndDebounce(ef, mb::Middle, true);
    ClearButtons(ef);

    // Idler and Selector are not on HOLD state
    REQUIRE(mi::idler.State() != mm::MovableBase::OnHold);
    REQUIRE(ms::selector.State() != mm::MovableBase::OnHold);

    // Error code is now OK
    // LEDs turn off at the ejected slot
    REQUIRE(VerifyState2(ef, mg::FilamentLoadState::AtPulley, mi::idler.IdleSlotIndex(), selectorParkedPos, false, false, ejectSlot, ml::off, ml::off, ErrorCode::OK, ProgressCode::OK));
}
