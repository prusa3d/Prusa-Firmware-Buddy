#include "catch2/catch_test_macros.hpp"
#include "catch2/generators/catch_generators.hpp"

#include <functional>

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

#include "../../../../src/logic/load_filament.h"

#include "../../modules/stubs/stub_adc.h"

#include "../stubs/homing.h"
#include "../stubs/main_loop_stub.h"
#include "../stubs/stub_motion.h"

using namespace std::placeholders;

#include "../helpers/helpers.ipp"

void LoadFilamentCommonSetup(uint8_t slot, logic::LoadFilament &lf, bool feedLimited) {
    ForceReinitAllAutomata();

    // change the startup to what we need here
    REQUIRE(EnsureActiveSlotIndex(slot, mg::FilamentLoadState::AtPulley));

    // verify startup conditions
    REQUIRE(VerifyState(lf, mg::FilamentLoadState::AtPulley, mi::Idler::IdleSlotIndex(), slot, false, false, ml::off, ml::off, ErrorCode::OK, ProgressCode::OK));

    // restart the automaton
    if (feedLimited) {
        lf.ResetLimited(slot);
    } else {
        lf.Reset(slot);
    }

    // Stage 0 - verify state just after Reset()
    // we assume the filament is not loaded
    // idler should have been activated by the underlying automaton
    // no change in selector's position
    // FINDA off
    // green LED should blink, red off
    REQUIRE(VerifyState(lf, mg::FilamentLoadState::AtPulley, mi::Idler::IdleSlotIndex(), slot, false, false, ml::blink0, ml::off, ErrorCode::RUNNING, ProgressCode::FeedingToFinda));
}

void LoadFilamentSuccessful(uint8_t slot, logic::LoadFilament &lf) {
    // Stage 2 - feeding to finda
    // we'll assume the finda is working correctly here
    REQUIRE(WhileCondition(
        lf,
        [&](uint32_t step) -> bool {
        if(step == 100){ // on 100th step make FINDA trigger
            hal::gpio::WritePin(FINDA_PIN, hal::gpio::Level::high);
        }
        return lf.TopLevelState() == ProgressCode::FeedingToFinda; },
        5000));
    REQUIRE(VerifyState(lf, mg::FilamentLoadState::InSelector, slot, slot, true, true, ml::blink0, ml::off, ErrorCode::RUNNING, ProgressCode::RetractingFromFinda));

    // Stage 3 - retracting from finda
    // we'll assume the finda is working correctly here
    REQUIRE(WhileCondition(
        lf,
        [&](uint32_t step) -> bool {
        if(step == 50){ // on 50th step make FINDA trigger
            hal::gpio::WritePin(FINDA_PIN, hal::gpio::Level::low);
        }
        return lf.TopLevelState() == ProgressCode::RetractingFromFinda; },
        5000));
    REQUIRE(VerifyState(lf, mg::FilamentLoadState::AtPulley, slot, slot, false, true, ml::off, ml::off, ErrorCode::RUNNING, ProgressCode::DisengagingIdler));

    // Stage 4 - disengaging idler
    REQUIRE(WhileTopState(lf, ProgressCode::DisengagingIdler, idlerEngageDisengageMaxSteps));
    REQUIRE(VerifyState(lf, mg::FilamentLoadState::AtPulley, mi::Idler::IdleSlotIndex(), slot, false, false, ml::off, ml::off, ErrorCode::OK, ProgressCode::OK));
}

void LoadFilamentSuccessfulWithRehomeSelector(uint8_t slot, logic::LoadFilament &lf) {
    // Stage 2 - feeding to finda
    // make FINDA switch on
    REQUIRE(WhileCondition(lf, std::bind(SimulateFeedToFINDA, _1, 100), 5000));
    REQUIRE(VerifyState(lf, mg::FilamentLoadState::InSelector, slot, slot, true, true, ml::blink0, ml::off, ErrorCode::RUNNING, ProgressCode::RetractingFromFinda));

    // Stage 3 - retracting from finda
    // we'll assume the finda is working correctly here
    REQUIRE(WhileCondition(lf, std::bind(SimulateRetractFromFINDA, _1, 100), 5000));
    REQUIRE(WhileCondition(
        lf, [&](uint32_t) { return lf.State() == ProgressCode::RetractingFromFinda; }, 50000));
    REQUIRE(VerifyState(lf, mg::FilamentLoadState::AtPulley, slot, 0xff, false, true, ml::off, ml::off, ErrorCode::RUNNING, ProgressCode::DisengagingIdler));

    // Stage 4 - disengaging Idler + homing Selector simultaneously
    SimulateSelectorHoming(lf);

    REQUIRE(VerifyState(lf, mg::FilamentLoadState::AtPulley, mi::Idler::IdleSlotIndex(), slot, false, false, ml::off, ml::off, ErrorCode::OK, ProgressCode::OK));
}

TEST_CASE("load_filament::regular_load_to_slot_0-4", "[load_filament]") {
    for (uint8_t slot = 0; slot < config::toolCount; ++slot) {
        logic::LoadFilament lf;
        LoadFilamentCommonSetup(slot, lf, true);
        LoadFilamentSuccessful(slot, lf);
    }
}

void FailedLoadToFinda(uint8_t slot, logic::LoadFilament &lf) {
    // Stage 2 - feeding to finda
    // we'll assume the finda is defective here and does not trigger
    REQUIRE(WhileTopState(lf, ProgressCode::FeedingToFinda, 50000));
    REQUIRE(VerifyState(lf, mg::FilamentLoadState::InSelector, slot, slot, false, true, ml::off, ml::blink0, ErrorCode::RUNNING, ProgressCode::ERRDisengagingIdler));

    // Stage 3 - disengaging idler in error mode
    SimulateErrDisengagingIdler(lf, ErrorCode::FINDA_DIDNT_SWITCH_ON);

    REQUIRE(VerifyState(lf, mg::FilamentLoadState::InSelector, mi::Idler::IdleSlotIndex(), slot, false, false, ml::off, ml::blink0, ErrorCode::FINDA_DIDNT_SWITCH_ON, ProgressCode::ERRWaitingForUser));
}

void FailedLoadToFindaResolveManual(uint8_t slot, logic::LoadFilament &lf) {
    // simulate the user fixed the issue himself

    // Perform press on button 2 + debounce + switch on FINDA
    SetFINDAStateAndDebounce(true);
    PressButtonAndDebounce(lf, mb::Right, false);

    // the Idler also engages in this call as this is planned as the next step
    SimulateIdlerHoming(lf);

    // pulling filament back
    REQUIRE(VerifyState(lf, mg::FilamentLoadState::InSelector, mi::Idler::IdleSlotIndex(), slot, true, true, ml::blink0, ml::off, ErrorCode::RUNNING, ProgressCode::RetractingFromFinda));

    ClearButtons(lf);

    // Stage 3 - retracting from finda
    // we'll assume the finda is working correctly here
    REQUIRE(WhileCondition(
        lf,
        [&](uint32_t step) -> bool {
        if(step == 50){ // on 50th step make FINDA trigger
            hal::gpio::WritePin(FINDA_PIN, hal::gpio::Level::low);
        }
        return lf.TopLevelState() == ProgressCode::RetractingFromFinda; },
        5000));

    // This is a tricky part as the Selector will start homing asynchronnously right after
    // the filament state switches to AtPulley.
    // The trouble is, that the filament state is updated after the Pulley finishes
    // its moves (which is correct), but we don't have enough cycles to home the selector afterwards
    // - basically it will just start homing
    // Moreover, the Idler is to disengage meanwhile, which makes the simulation even harder.
    // Therefore we just tick the StallGuard of the Selector and hope for the best
    //
    // With the introduction of dual-side homing, the simulation gets even harder,
    // so let's assume the MMU does its job -> prefer simulating selector homing properly and check the machine's state afterwards
    SimulateSelectorHoming(lf);

    // just one step is necessary to "finish" homing
    // but the selector then (correctly) plans its move to the original position
    // therefore we expect the selector to have its idle position at this stage
    // REQUIRE(VerifyState(lf, mg::FilamentLoadState::AtPulley, slot, ms::selector.IdleSlotIndex(), false, true, ml::off, ml::off, ErrorCode::RUNNING, ProgressCode::DisengagingIdler));

    // disengaging idler (and the selector will move to the desired position meanwhile
    REQUIRE(WhileTopState(lf, ProgressCode::DisengagingIdler, idlerEngageDisengageMaxSteps));
    REQUIRE(VerifyState(lf, mg::FilamentLoadState::AtPulley, mi::Idler::IdleSlotIndex(), slot, false, false, ml::off, ml::off, ErrorCode::OK, ProgressCode::OK));
}

void FailedLoadToFindaResolveTryAgain(uint8_t slot, logic::LoadFilament &lf) {
    PressButtonAndDebounce(lf, mb::Middle, false);

    // the state machine should have restarted
    // Idler's position needs to be ignored as it has started homing after the button press
    REQUIRE(VerifyState(lf, mg::FilamentLoadState::InSelector, config::toolCount, slot, false, false, ml::blink0, ml::off, ErrorCode::RUNNING, ProgressCode::FeedingToFinda));
    ClearButtons(lf);

    SimulateIdlerHoming(lf);

    LoadFilamentSuccessfulWithRehomeSelector(slot, lf);
}

TEST_CASE("load_filament::invalid_slot", "[load_filament]") {
    for (uint8_t activeSlot = 0; activeSlot < config::toolCount; ++activeSlot) {
        logic::LoadFilament lf;
        InvalidSlot<logic::LoadFilament>(lf, activeSlot, config::toolCount);
    }
}

TEST_CASE("load_filament::state_machine_reusal", "[load_filament]") {
    logic::LoadFilament lf;

    for (uint8_t fromSlot = 0; fromSlot < config::toolCount; ++fromSlot) {
        for (uint8_t toSlot = 0; toSlot < config::toolCount + 2; ++toSlot) {
            logic::LoadFilament lf;
            if (toSlot >= config::toolCount) {
                InvalidSlot<logic::LoadFilament>(lf, fromSlot, toSlot);
            } else {
                LoadFilamentCommonSetup(toSlot, lf, true);
                LoadFilamentSuccessful(toSlot, lf);
            }
        }
    }
}

TEST_CASE("load_filament::failed_load_to_finda_0-4_try_again", "[load_filament]") {
    for (uint8_t slot = 0; slot < config::toolCount; ++slot) {
        logic::LoadFilament lf;
        LoadFilamentCommonSetup(slot, lf, true);
        FailedLoadToFinda(slot, lf);
        FailedLoadToFindaResolveTryAgain(slot, lf);
    }
}

void LoadFilamentSuccessWithRecheck(uint8_t slot, logic::LoadFilament &lf) {
    // Stage 2 - feeding to finda
    // we'll assume the finda is working correctly here
    REQUIRE(WhileCondition(
        lf,
        [&](uint32_t step) -> bool {
            if(step == 100){ // on 100th step make FINDA trigger
                hal::gpio::WritePin(FINDA_PIN, hal::gpio::Level::high);
            }
            return lf.TopLevelState() == ProgressCode::FeedingToFinda; },
        5000));
    REQUIRE(VerifyState(lf, mg::FilamentLoadState::InSelector, slot, slot, true, true, ml::blink0, ml::off, ErrorCode::RUNNING, ProgressCode::RetractingFromFinda));

    // Stage 3 - retracting from finda
    // we'll assume the finda is working correctly here
    REQUIRE(WhileCondition(
        lf,
        [&](uint32_t step) -> bool {
            if(step == 50){ // on 50th step make FINDA trigger
                hal::gpio::WritePin(FINDA_PIN, hal::gpio::Level::low);
            }
            return lf.TopLevelState() == ProgressCode::RetractingFromFinda; },
        5000));
    REQUIRE(VerifyState(lf, mg::FilamentLoadState::AtPulley, slot, slot, false, true, ml::blink0, ml::off, ErrorCode::RUNNING, ProgressCode::FeedingToFinda));

    // make FINDA switch on
    REQUIRE(WhileCondition(lf, std::bind(SimulateFeedToFINDA, _1, 100), 5000));
    REQUIRE(VerifyState(lf, mg::FilamentLoadState::InSelector, slot, slot, true, true, ml::blink0, ml::off, ErrorCode::RUNNING, ProgressCode::RetractingFromFinda));

    // make FINDA switch off
    REQUIRE(WhileCondition(lf, std::bind(SimulateRetractFromFINDA, _1, 100), 5000));
    REQUIRE(WhileCondition(
        lf, [&](uint32_t) { return lf.State() == ProgressCode::RetractingFromFinda; }, 50000));
}

TEST_CASE("load_filament::unlimited_load", "[load_filament]") {
    for (uint8_t slot = 0; slot < config::toolCount; ++slot) {
        logic::LoadFilament lf;
        LoadFilamentCommonSetup(slot, lf, false);
        LoadFilamentSuccessWithRecheck(slot, lf);
    }
}

void LoadFilamentStopped(uint8_t slot, logic::LoadFilament &lf) {
    // Stage 2 - feeding to finda
    // we'll assume the finda is working correctly here, but we stop the load sequence with a button press
    // just step a bit
    REQUIRE_FALSE(WhileTopState(lf, ProgressCode::FeedingToFinda, 5000));

    // now press a button
    PressButtonAndDebounce(lf, mb::Middle, false);

    REQUIRE(VerifyState(lf, mg::FilamentLoadState::InSelector, slot, slot, false, true, ml::blink0, ml::off, ErrorCode::RUNNING, ProgressCode::RetractingFromFinda));

    // Stage 3 - retracting from finda, but no FINDA trigger since we stopped the load with a button press
    // - actually, we should be safe though as FINDA is not triggered at all
    REQUIRE(WhileTopState(lf, ProgressCode::RetractingFromFinda));
    REQUIRE(VerifyState(lf, mg::FilamentLoadState::AtPulley, slot, slot, false, true, ml::off, ml::off, ErrorCode::RUNNING, ProgressCode::DisengagingIdler));

    // Stage 4 - disengaging idler
    REQUIRE(WhileTopState(lf, ProgressCode::DisengagingIdler, idlerEngageDisengageMaxSteps));
    REQUIRE(VerifyState(lf, mg::FilamentLoadState::AtPulley, mi::Idler::IdleSlotIndex(), slot, false, false, ml::off, ml::off, ErrorCode::OK, ProgressCode::OK));
}

TEST_CASE("load_filament::unlimited_load_manual_stop", "[load_filament]") {
    for (uint8_t slot = 0; slot < config::toolCount; ++slot) {
        logic::LoadFilament lf;
        LoadFilamentCommonSetup(slot, lf, false);
        LoadFilamentStopped(slot, lf);
    }
}

void LoadFilamentAlreadyPresentFilament(uint8_t slot, logic::LoadFilament &lf) {
    //one of the first steps of the state machine should pick up the fact that FINDA is on and transfer into the retracting phase
    REQUIRE(WhileTopState(lf, ProgressCode::FeedingToFinda, 5000));
    REQUIRE(VerifyState(lf, mg::FilamentLoadState::InSelector, slot, slot, true, true, ml::blink0, ml::off, ErrorCode::RUNNING, ProgressCode::RetractingFromFinda));
    REQUIRE(WhileCondition(lf, std::bind(SimulateRetractFromFINDA, _1, 100), 5000));
    REQUIRE(WhileTopState(lf, ProgressCode::RetractingFromFinda, 5000));
    REQUIRE(VerifyState(lf, mg::FilamentLoadState::AtPulley, slot, slot, false, true, ml::blink0, ml::off, ErrorCode::RUNNING, ProgressCode::FeedingToFinda));
    // make FINDA switch on again
    REQUIRE(WhileCondition(lf, std::bind(SimulateFeedToFINDA, _1, 100), 5000));
    REQUIRE(WhileTopState(lf, ProgressCode::FeedingToFinda, 5000));
    REQUIRE(VerifyState(lf, mg::FilamentLoadState::InSelector, slot, slot, true, true, ml::blink0, ml::off, ErrorCode::RUNNING, ProgressCode::RetractingFromFinda));
    // make FINDA switch off again
    REQUIRE(WhileCondition(lf, std::bind(SimulateRetractFromFINDA, _1, 100), 5000));
    REQUIRE(WhileTopState(lf, ProgressCode::RetractingFromFinda, 5000));
}

TEST_CASE("load_filament::avoid_load_filament_finda", "[load_filament]") {
    auto fls = GENERATE(mg::FilamentLoadState::InSelector, mg::FilamentLoadState::InFSensor, mg::FilamentLoadState::InNozzle);

    for (uint8_t slot = 0; slot < config::toolCount; ++slot) {
        for (uint8_t activeSlot = 0; activeSlot < config::toolCount; ++activeSlot) {
            logic::LoadFilament lf;
            ForceReinitAllAutomata();
            REQUIRE(EnsureActiveSlotIndex(activeSlot, fls));
            SetFINDAStateAndDebounce(true); // beware - selector will refuse to move if FINDA is pressed - must set active slot first and then FINDA
            REQUIRE(VerifyState(lf, fls, mi::Idler::IdleSlotIndex(), activeSlot, true, false, ml::off, ml::off, ErrorCode::OK, ProgressCode::OK));
            bool accepted = lf.Reset(slot);
            if (activeSlot != slot) {
                REQUIRE_FALSE(accepted);
            } else if (activeSlot == slot && fls <= mg::FilamentLoadState::InSelector) {
                LoadFilamentAlreadyPresentFilament(slot, lf);
            } else {
                REQUIRE_FALSE(accepted);
            }
        }
    }
}
