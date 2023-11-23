#include "catch2/catch_test_macros.hpp"

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

#include "../../../../src/logic/unload_filament.h"

#include "../../modules/stubs/stub_adc.h"

#include "../stubs/homing.h"
#include "../stubs/main_loop_stub.h"
#include "../stubs/stub_motion.h"

using namespace std::placeholders;

#include "../helpers/helpers.ipp"

void RegularUnloadFromSlot04Init(uint8_t slot, logic::UnloadFilament &uf) {
    // prepare startup conditions
    ForceReinitAllAutomata();

    // change the startup to what we need here
    REQUIRE(EnsureActiveSlotIndex(slot, mg::FilamentLoadState::InNozzle));

    // set FINDA ON + debounce
    SetFINDAStateAndDebounce(true);

    // verify startup conditions
    REQUIRE(VerifyState(uf, mg::FilamentLoadState::InNozzle, mi::Idler::IdleSlotIndex(), slot, true, false, ml::off, ml::off, ErrorCode::OK, ProgressCode::OK));

    // restart the automaton
    uf.Reset(slot);
}

void RegularUnloadFromSlot04(uint8_t slot, logic::UnloadFilament &uf, uint8_t entryIdlerSlotIndex,
    bool selectorShallHomeAtEnd, ml::Mode entryGreenLED) {
    // Stage 0 - verify state just after Reset()
    // we still think we have filament loaded at this stage
    // idler should have been activated by the underlying automaton
    // no change in selector's position
    // FINDA on
    // green LED should blink, red off
    REQUIRE(VerifyState(uf, (mg::FilamentLoadState)(mg::FilamentLoadState::InNozzle | mg::FilamentLoadState::InSelector),
        entryIdlerSlotIndex, slot, true, true, entryGreenLED, ml::off, ErrorCode::RUNNING, ProgressCode::UnloadingToFinda));

    // run the automaton
    // Stage 1 - unloading to FINDA
    REQUIRE(WhileCondition(
        uf,
        [&](uint32_t step) -> bool {
        if(step == 100){ // on 100th step make FINDA trigger
            hal::gpio::WritePin(FINDA_PIN, hal::gpio::Level::low);
        }
        return uf.TopLevelState() == ProgressCode::UnloadingToFinda; },
        50000));

    // we still think we have filament loaded at this stage
    // idler should have been activated by the underlying automaton
    // no change in selector's position
    // FINDA triggered off
    // green LED should be off
    REQUIRE(VerifyState(uf, mg::FilamentLoadState::InSelector, slot, slot, false, true, ml::blink0, ml::off, ErrorCode::RUNNING, ProgressCode::RetractingFromFinda));

    // Stage 2 - retracting from FINDA
    REQUIRE(WhileTopState(uf, ProgressCode::RetractingFromFinda, idlerEngageDisengageMaxSteps));

    if (selectorShallHomeAtEnd) {
        REQUIRE(ms::selector.Slot() == 0xff);
        SimulateSelectorHoming(uf);
    }

    // Stage 3 - idler was engaged, disengage it
    REQUIRE(WhileTopState(uf, ProgressCode::DisengagingIdler, idlerEngageDisengageMaxSteps));

    // filament unloaded
    // idler should have been disengaged
    // no change in selector's position
    // FINDA still triggered off
    // green LED should be off
    REQUIRE(VerifyState(uf, mg::FilamentLoadState::AtPulley, mi::Idler::IdleSlotIndex(), slot, false, false, ml::off, ml::off, ErrorCode::OK, ProgressCode::OK));

    // Stage 5 - repeated calls to TopLevelState should return "OK"
    REQUIRE(uf.TopLevelState() == ProgressCode::OK);
    REQUIRE(mg::globals.FilamentLoaded() == mg::FilamentLoadState::AtPulley);
    REQUIRE(mf::finda.Pressed() == false);
    REQUIRE(uf.Error() == ErrorCode::OK); // no error
}

TEST_CASE("unload_filament::regular_unload_from_slot_0-4", "[unload_filament]") {
    for (uint8_t slot = 0; slot < config::toolCount; ++slot) {
        logic::UnloadFilament uf;
        RegularUnloadFromSlot04Init(slot, uf);
        RegularUnloadFromSlot04(slot, uf, mi::Idler::IdleSlotIndex(), false, ml::off);
    }
}

void FindaDidntTriggerCommonSetup(uint8_t slot, logic::UnloadFilament &uf) {
    // prepare startup conditions
    ForceReinitAllAutomata();

    // change the startup to what we need here
    // move selector to the right spot
    REQUIRE(EnsureActiveSlotIndex(slot, mg::FilamentLoadState::InNozzle));

    // set FINDA ON + debounce
    SetFINDAStateAndDebounce(true);

    //    mg::globals.SetFilamentLoaded(mg::globals.ActiveSlot(), mg::FilamentLoadState::InNozzle);

    // verify startup conditions
    REQUIRE(VerifyState(uf, mg::FilamentLoadState::InNozzle, mi::Idler::IdleSlotIndex(), slot, true, false, ml::off, ml::off, ErrorCode::OK, ProgressCode::OK));

    // restart the automaton
    uf.Reset(slot);

    // Stage 0 - verify state just after Reset()
    // we still think we have filament loaded at this stage
    // idler should have been activated by the underlying automaton
    // no change in selector's position
    // FINDA triggered off
    // green LED should be off
    // no error so far
    REQUIRE(VerifyState(uf, mg::FilamentLoadState::InNozzle, mi::Idler::IdleSlotIndex(), slot, true, true, ml::off, ml::off, ErrorCode::RUNNING, ProgressCode::UnloadingToFinda));

    // run the automaton
    // Stage 1 - unloading to FINDA - do NOT let it trigger - keep it pressed, the automaton should finish all moves with the pulley
    // without reaching the FINDA and report an error
    REQUIRE(WhileCondition(
        uf,
        [&](uint32_t step) {
            SimulateUnloadToFINDA(step, 10, 1'000'000);
            return uf.TopLevelState() == ProgressCode::UnloadingToFinda;
        },
        200'000));

    // we still think we have filament loaded at this stage
    // idler should have been activated by the underlying automaton
    // no change in selector's position
    // FINDA still on
    // red LED should blink, green LED should be off
    REQUIRE(VerifyState(uf, mg::FilamentLoadState::InSelector, slot, slot, true, true, ml::off, ml::blink0, ErrorCode::RUNNING, ProgressCode::ERRDisengagingIdler));

    // Stage 2 - idler should get disengaged
    SimulateErrDisengagingIdler(uf, ErrorCode::FINDA_DIDNT_SWITCH_OFF);

    // we still think we have filament loaded at this stage
    // idler should have been disengaged
    // no change in selector's position
    // FINDA still on
    // red LED should blink
    // green LED should be off
    REQUIRE(VerifyState(uf, mg::FilamentLoadState::InSelector, mi::Idler::IdleSlotIndex(), slot, true, false, ml::off, ml::blink0, ErrorCode::FINDA_DIDNT_SWITCH_OFF, ProgressCode::ERRWaitingForUser));
}

void FindaDidntTriggerResolveTryAgain(uint8_t slot, logic::UnloadFilament &uf) {
    // Stage 3 - the user has to do something
    // there are 3 options:
    // - help the filament a bit
    // - try again the whole sequence
    // - resolve the problem by hand - after pressing the button we shall check, that FINDA is off and we should do what?

    // In this case we check the second option
    PressButtonAndDebounce(uf, mb::Middle, false);

    // we still think we have filament loaded at this stage
    // idler should have been disengaged
    // no change in selector's position
    // FINDA still on
    // red LED should blink, green LED should be off
    REQUIRE(VerifyState(uf, mg::FilamentLoadState::InSelector, mi::Idler::IdleSlotIndex(), slot, true, true, ml::off, ml::off, ErrorCode::RUNNING, ProgressCode::UnloadingToFinda));

    ClearButtons(uf);

    // Assume, the Idler homed (homing is invalidated after pressing the recovery button)
    SimulateIdlerHoming(uf);

    // Wait for the idler homing to become valid, and for the
    // idler to return to 'Ready' state
    SimulateIdlerMoveToParkingPosition(uf);
}

TEST_CASE("unload_filament::finda_didnt_trigger_resolve_try_again", "[unload_filament]") {
    for (uint8_t slot = 0; slot < config::toolCount; ++slot) {
        logic::UnloadFilament uf;
        FindaDidntTriggerCommonSetup(slot, uf);
        FindaDidntTriggerResolveTryAgain(slot, uf);
        RegularUnloadFromSlot04(slot, uf, slot, true, ml::blink0);
    }
}

TEST_CASE("unload_filament::not_loaded", "[unload_filament]") {
    logic::UnloadFilament uf;

    // prepare startup conditions
    ForceReinitAllAutomata();

    // change the startup to what we need here
    // move selector to the right spot
    REQUIRE(EnsureActiveSlotIndex(0, mg::FilamentLoadState::AtPulley));

    // verify startup conditions
    REQUIRE(VerifyState(uf, mg::FilamentLoadState::AtPulley, mi::Idler::IdleSlotIndex(), 0, false, false, ml::off, ml::off, ErrorCode::OK, ProgressCode::OK));

    // restart the automaton
    uf.Reset(0);

    // Stage 0 - unload filament should finish immediately as there is no filament loaded
    REQUIRE(VerifyState(uf, mg::FilamentLoadState::AtPulley, mi::Idler::IdleSlotIndex(), 0, false, false, ml::off, ml::off, ErrorCode::OK, ProgressCode::OK));
}

void FailedUnloadResolveManual(uint8_t slot, logic::UnloadFilament &uf) {
    // simulate the user fixed the issue himself

    // Perform press on button 2 + debounce + switch off FINDA
    hal::gpio::WritePin(FINDA_PIN, hal::gpio::Level::low);
    PressButtonAndDebounce(uf, mb::Right, false);

    REQUIRE(VerifyState(uf, mg::FilamentLoadState::InSelector, mi::Idler::IdleSlotIndex(), slot, false, false, ml::blink0, ml::off, ErrorCode::RUNNING, ProgressCode::FeedingToFinda));

    // we still need to feed to FINDA and back to verify the position of the filament
    SimulateIdlerHoming(uf);

    REQUIRE(WhileCondition(uf, std::bind(SimulateFeedToFINDA, _1, 100), 5000));

    REQUIRE(WhileCondition(uf, std::bind(SimulateRetractFromFINDA, _1, 100), 5000));
    REQUIRE(WhileCondition(
        uf, [&](uint32_t) { return uf.State() == ProgressCode::RetractingFromFinda; }, 50000));

    REQUIRE(VerifyState(uf, mg::FilamentLoadState::AtPulley, config::toolCount, config::toolCount, false, true, ml::off, ml::off, ErrorCode::RUNNING, ProgressCode::DisengagingIdler));
    SimulateSelectorHoming(uf);
    REQUIRE(WhileTopState(uf, ProgressCode::DisengagingIdler, idlerEngageDisengageMaxSteps));

    REQUIRE(VerifyState(uf, mg::FilamentLoadState::AtPulley, mi::Idler::IdleSlotIndex(), slot, false, false, ml::off, ml::off, ErrorCode::OK, ProgressCode::OK));
}

void FailedUnloadResolveManualFINDAon(uint8_t slot, logic::UnloadFilament &uf) {
    // simulate the user fixed the issue himself

    // Perform press on button 2 + debounce + keep FINDA on
    PressButtonAndDebounce(uf, mb::Right, false);

    REQUIRE(VerifyState(uf, mg::FilamentLoadState::InSelector, mi::Idler::IdleSlotIndex(), slot, true, false, ml::off, ml::blink0, ErrorCode::FINDA_DIDNT_SWITCH_OFF, ProgressCode::ERRWaitingForUser));
}

void FailedUnloadResolveManualFSensorOn(uint8_t slot, logic::UnloadFilament &uf) {
    // simulate the user fixed the issue himself

    // Perform press on button 2 + debounce + keep FSensor on
    SetFSensorStateAndDebounce(true);
    hal::gpio::WritePin(FINDA_PIN, hal::gpio::Level::low);
    PressButtonAndDebounce(uf, mb::Right, false);

    REQUIRE(VerifyState(uf, mg::FilamentLoadState::InSelector, mi::Idler::IdleSlotIndex(), slot, false, false, ml::off, ml::blink0, ErrorCode::FSENSOR_DIDNT_SWITCH_OFF, ProgressCode::ERRWaitingForUser));
}

TEST_CASE("unload_filament::failed_unload_to_finda_0-4_resolve_manual_FINDA_on", "[unload_filament]") {
    for (uint8_t slot = 0; slot < config::toolCount; ++slot) {
        logic::UnloadFilament uf;
        FindaDidntTriggerCommonSetup(slot, uf);
        FailedUnloadResolveManualFINDAon(slot, uf);
    }
}
