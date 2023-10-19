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
#include "../../../../src/modules/user_input.h"

#include "../../../../src/logic/tool_change.h"

#include "../../modules/stubs/stub_adc.h"
#include "../../modules/stubs/stub_timebase.h"

#include "../stubs/homing.h"
#include "../stubs/main_loop_stub.h"
#include "../stubs/stub_motion.h"

#include <functional>

using namespace std::placeholders;

#include "../helpers/helpers.ipp"

// needs to be a separate function otherwise gdb has issues setting breakpoints inside
bool FeedingToFindaStep(logic::CommandBase &tc, uint32_t step, uint32_t triggerAt) {
    if (step == triggerAt) { // on specified stepNr make FINDA trigger
        hal::gpio::WritePin(FINDA_PIN, hal::gpio::Level::high);
    } else if (step >= triggerAt + config::findaDebounceMs + 2) {
        REQUIRE(mf::finda.Pressed() == true);
    }
    return tc.TopLevelState() == ProgressCode::FeedingToFinda;
}

void FeedingToFinda(logic::ToolChange &tc, uint8_t toSlot, uint32_t triggerAt = 1000) { // feeding to finda
    REQUIRE(WhileCondition(tc, std::bind(FeedingToFindaStep, std::ref(tc), _1, triggerAt), 200'000UL));
    REQUIRE(VerifyState(tc, mg::FilamentLoadState::InSelector, toSlot, toSlot, true, true, ml::blink0, ml::off, ErrorCode::RUNNING, ProgressCode::FeedingToBondtech));
}

void FeedingToBondtech(logic::ToolChange &tc, uint8_t toSlot) {
    // james is feeding fast and then slowly
    // FSensor must not trigger too early
    REQUIRE_FALSE(mfs::fsensor.Pressed());
    REQUIRE(WhileCondition(
        tc,
        [&](uint32_t step) -> bool {
        if(step == mm::unitToSteps<mm::P_pos_t>(config::minimumBowdenLength)+10){ // on the correct step make filament sensor trigger
            mfs::fsensor.ProcessMessage(true);
        }
        return tc.TopLevelState() == ProgressCode::FeedingToBondtech; },
        mm::unitToSteps<mm::P_pos_t>(config::minimumBowdenLength) + 10000));
    REQUIRE(VerifyState(tc, mg::FilamentLoadState::InNozzle, mi::Idler::IdleSlotIndex(), toSlot, true, false, ml::on, ml::off, ErrorCode::OK, ProgressCode::OK));
}

void CheckFinishedCorrectly(logic::ToolChange &tc, uint8_t toSlot) {
    REQUIRE(tc.TopLevelState() == ProgressCode::OK);
    REQUIRE(mg::globals.FilamentLoaded() == mg::FilamentLoadState::InNozzle);
    REQUIRE(mg::globals.ActiveSlot() == toSlot);
}

// This function exists for the sole purpose of debugging.
// WritePin is always_inline and gdb has a hard time settinge breakpoints when FINDA should do something
void FINDAOnOff(bool press) {
    hal::gpio::WritePin(FINDA_PIN, press ? hal::gpio::Level::high : hal::gpio::Level::low);
}

bool SimulateUnloadFilament(uint32_t step, const logic::CommandBase *tc, uint32_t unloadLengthSteps) {
    if (step == 20) { // on 20th step make FSensor switch off
        mfs::fsensor.ProcessMessage(false);
    } else if (step == unloadLengthSteps) {
        FINDAOnOff(false);
    }
    return tc->TopLevelState() == ProgressCode::UnloadingFilament;
}

void ToolChange(logic::ToolChange &tc, uint8_t fromSlot, uint8_t toSlot) {
    ForceReinitAllAutomata();
    SetMinimalBowdenLength();

    REQUIRE(EnsureActiveSlotIndex(fromSlot, mg::FilamentLoadState::InNozzle));
    SetFINDAStateAndDebounce(true);
    SetFSensorStateAndDebounce(true);

    // restart the automaton
    tc.Reset(toSlot);

    REQUIRE(WhileCondition(
        tc,
        std::bind(SimulateUnloadFilament, _1, &tc, mm::unitToSteps<mm::P_pos_t>(config::minimumBowdenLength)),
        200'000UL));

    //    REQUIRE(mg::globals.FilamentLoaded() == mg::FilamentLoadState::AtPulley);
    REQUIRE(VerifyState2(tc, mg::FilamentLoadState::AtPulley, mi::Idler::IdleSlotIndex(), fromSlot, false, false, toSlot, ml::blink0, ml::off, ErrorCode::RUNNING, ProgressCode::FeedingToFinda));

    FeedingToFinda(tc, toSlot);

    FeedingToBondtech(tc, toSlot);

    CheckFinishedCorrectly(tc, toSlot);
}

void NoToolChange(logic::ToolChange &tc, uint8_t fromSlot, uint8_t toSlot) {
    ForceReinitAllAutomata();
    SetMinimalBowdenLength();

    REQUIRE(EnsureActiveSlotIndex(fromSlot, mg::FilamentLoadState::InNozzle));
    // the filament is LOADED
    SetFINDAStateAndDebounce(true);
    SetFSensorStateAndDebounce(true);

    REQUIRE(VerifyEnvironmentState(mg::FilamentLoadState::InNozzle, mi::Idler::IdleSlotIndex(), toSlot, true, false, ml::off, ml::off));

    // restart the automaton
    tc.Reset(toSlot);

    // should not do anything
    REQUIRE(tc.TopLevelState() == ProgressCode::OK);
    REQUIRE(tc.Error() == ErrorCode::OK);
}

void JustLoadFilament(logic::ToolChange &tc, uint8_t slot) {
    for (uint8_t startSelectorSlot = 0; startSelectorSlot < config::toolCount; ++startSelectorSlot) {
        ForceReinitAllAutomata();
        SetMinimalBowdenLength();
        // make sure all the modules are ready
        // MMU-196: Move selector to a "random" slot
        REQUIRE(EnsureActiveSlotIndex(startSelectorSlot, mg::FilamentLoadState::AtPulley));

        // verify filament NOT loaded
        REQUIRE(VerifyEnvironmentState(mg::FilamentLoadState::AtPulley, mi::Idler::IdleSlotIndex(), startSelectorSlot, false, false, ml::off, ml::off));

        // restart the automaton
        tc.Reset(slot);

        REQUIRE(ms::selector.plannedSlot == slot); // MMU-196 - make sure the selector is about to move to the desired slot

        FeedingToFinda(tc, slot);

        FeedingToBondtech(tc, slot);

        CheckFinishedCorrectly(tc, slot);
    }
}

TEST_CASE("tool_change::test0", "[tool_change]") {
    for (uint8_t fromSlot = 0; fromSlot < config::toolCount; ++fromSlot) {
        for (uint8_t toSlot = 0; toSlot < config::toolCount; ++toSlot) {
            logic::ToolChange tc;
            if (fromSlot != toSlot) {
                ToolChange(tc, fromSlot, toSlot);
            } else {
                NoToolChange(tc, fromSlot, toSlot);
            }
        }
    }
}

TEST_CASE("tool_change::invalid_slot", "[tool_change]") {
    for (uint8_t fromSlot = 0; fromSlot < config::toolCount; ++fromSlot) {
        logic::ToolChange tc;
        InvalidSlot<logic::ToolChange>(tc, fromSlot, config::toolCount);
    }
}

TEST_CASE("tool_change::state_machine_reusal", "[tool_change]") {
    logic::ToolChange tc;

    for (uint8_t fromSlot = 0; fromSlot < config::toolCount; ++fromSlot) {
        for (uint8_t toSlot = 0; toSlot < config::toolCount + 2; ++toSlot) {
            if (toSlot >= config::toolCount) {
                InvalidSlot<logic::ToolChange>(tc, fromSlot, toSlot);
            } else if (fromSlot != toSlot) {
                ToolChange(tc, fromSlot, toSlot);
            } else {
                NoToolChange(tc, fromSlot, toSlot);
            }
        }
    }
}

TEST_CASE("tool_change::same_slot_just_unloaded_filament", "[tool_change]") {
    for (uint8_t toSlot = 0; toSlot < config::toolCount; ++toSlot) {
        logic::ToolChange tc;
        JustLoadFilament(tc, toSlot);
    }
}

void ToolChangeFailLoadToFinda(logic::ToolChange &tc, uint8_t fromSlot, uint8_t toSlot) {
    ForceReinitAllAutomata();
    SetMinimalBowdenLength();

    REQUIRE(EnsureActiveSlotIndex(fromSlot, mg::FilamentLoadState::InNozzle));
    SetFINDAStateAndDebounce(true);
    SetFSensorStateAndDebounce(true);

    // restart the automaton
    tc.Reset(toSlot);

    REQUIRE(WhileCondition(tc, std::bind(SimulateUnloadToFINDA, _1, 100, 2'000), 200'000));
    REQUIRE(WhileTopState(tc, ProgressCode::UnloadingFilament, 5000));

    REQUIRE(mg::globals.FilamentLoaded() == mg::FilamentLoadState::AtPulley);

    // feeding to finda, but fails - do not trigger FINDA
    REQUIRE(WhileTopState(tc, ProgressCode::FeedingToFinda, 50000UL));

    // should end up in error disengage idler
    REQUIRE(VerifyState(tc, mg::FilamentLoadState::InSelector, toSlot, toSlot, false, true, ml::off, ml::blink0, ErrorCode::RUNNING, ProgressCode::ERRDisengagingIdler));

    SimulateErrDisengagingIdler(tc, ErrorCode::FINDA_DIDNT_SWITCH_ON);
}

void ToolChangeFailLoadToFindaMiddleBtn(logic::ToolChange &tc, uint8_t toSlot) {
    // now waiting for user input
    REQUIRE_FALSE(mui::userInput.AnyEvent());
    PressButtonAndDebounce(tc, mb::Middle, true);

    REQUIRE_FALSE(mi::idler.HomingValid());
    REQUIRE_FALSE(ms::selector.HomingValid());

    // We've entered FeedToFinda state machine
    REQUIRE(tc.TopLevelState() == ProgressCode::FeedingToFinda);

    // Idler homes first
    SimulateIdlerHoming(tc);
    SimulateIdlerMoveToParkingPosition(tc);

    // Now home the selector
    SimulateSelectorHoming(tc);

    // Wait for Selector to return to the planned slot
    REQUIRE(WhileCondition(
        tc, [&](uint32_t) {
            return tc.feed.State() == tc.feed.EngagingIdler;
        },
        selectorMoveMaxSteps));

    // @@TODO here is nasty disrepancy - FINDA is not pressed, but we pretend to have something in the Selector.
    //
    // 3 scenarios could have caused this:
    // 1. MMU was really unable to pick the filament piece and really failed to push anything into the Selector/FINDA
    // 2. MMU was able to pick a piece and managed to push it into the Selector, but not far enough to press FINDA
    // 3. FINDA is miscalibrated/defective and we have no idea that a piece of filament occurred below FINDA
    //
    // Scenarios 2 and 3 will cause serious problems, because the Selector is trying to rehome after "Retry"
    // I.e. FINDA state does not correspond to mg::globals::filamentLoaded (which is allowed and expected, but dangerous)
    //
    // Therefore we temporarily allow both AtPulley and InSelector.
    REQUIRE(VerifyState(tc,
        (mg::FilamentLoadState)(mg::FilamentLoadState::AtPulley | mg::FilamentLoadState::InSelector),
        mi::Idler::IdleSlotIndex(), toSlot, false, true, ml::blink0, ml::off, ErrorCode::RUNNING, ProgressCode::FeedingToFinda));

    ClearButtons(tc);

    // retry the whole operation
    // beware - the FeedToFinda state machine will leverage the already engaged Idler,
    // so the necessary number of steps to reach the FINDA is quite low (~250 was lowest once tested)
    // without running short of max distance of Pulley to travel
    FeedingToFinda(tc, toSlot, 250);

    FeedingToBondtech(tc, toSlot);

    CheckFinishedCorrectly(tc, toSlot);
}

void ToolChangeFailLoadToFindaRightBtnFINDA_FSensor(logic::ToolChange &tc, uint8_t toSlot) {
    // now waiting for user input - press FINDA and FSensor
    SetFINDAStateAndDebounce(true);
    REQUIRE(mf::finda.Pressed());
    SetFSensorStateAndDebounce(true);
    REQUIRE(mfs::fsensor.Pressed());

    REQUIRE_FALSE(mui::userInput.AnyEvent());
    PressButtonAndDebounce(tc, mb::Right, true);

    CheckFinishedCorrectly(tc, toSlot);

    ClearButtons(tc);
}

void ToolChangeFailLoadToFindaRightBtnFINDA(logic::ToolChange &tc, uint8_t toSlot) {
    // now waiting for user input - press FINDA
    SetFINDAStateAndDebounce(true);

    REQUIRE_FALSE(mui::userInput.AnyEvent());
    PressButtonAndDebounce(tc, mb::Right, true);

    REQUIRE(VerifyState(tc, mg::FilamentLoadState::InSelector, mi::Idler::IdleSlotIndex(), toSlot, true, false, ml::off, ml::blink0, ErrorCode::FSENSOR_DIDNT_SWITCH_ON, ProgressCode::ERRWaitingForUser));

    ClearButtons(tc);
}

void ToolChangeFailLoadToFindaRightBtn(logic::ToolChange &tc, uint8_t toSlot) {
    // now waiting for user input - do not press anything
    REQUIRE_FALSE(mui::userInput.AnyEvent());
    PressButtonAndDebounce(tc, mb::Right, true);

    REQUIRE(VerifyState(tc, mg::FilamentLoadState::InSelector, mi::Idler::IdleSlotIndex(), toSlot, false, false, ml::off, ml::blink0, ErrorCode::FINDA_DIDNT_SWITCH_ON, ProgressCode::ERRWaitingForUser));

    ClearButtons(tc);
}

TEST_CASE("tool_change::load_fail_FINDA_resolve_btnM", "[tool_change]") {
    logic::ToolChange tc;
    for (uint8_t fromSlot = 0; fromSlot < config::toolCount; ++fromSlot) {
        for (uint8_t toSlot = 0; toSlot < config::toolCount; ++toSlot) {
            if (fromSlot != toSlot) {
                ToolChangeFailLoadToFinda(tc, fromSlot, toSlot);
                ToolChangeFailLoadToFindaMiddleBtn(tc, toSlot);
            }
        }
    }
}

void ToolChangeFailFSensor(logic::ToolChange &tc, uint8_t fromSlot, uint8_t toSlot) {
    using namespace std::placeholders;
    ForceReinitAllAutomata();
    SetMinimalBowdenLength();

    REQUIRE(EnsureActiveSlotIndex(fromSlot, mg::FilamentLoadState::InNozzle));
    SetFINDAStateAndDebounce(true);
    SetFSensorStateAndDebounce(true);

    // restart the automaton
    tc.Reset(toSlot);

    REQUIRE(VerifyState(tc, mg::FilamentLoadState::InNozzle, mi::idler.IdleSlotIndex(), fromSlot, true, true, ml::off, ml::off, ErrorCode::RUNNING, ProgressCode::UnloadingFilament));
    // simulate unload to finda but fail the fsensor test
    REQUIRE(WhileCondition(tc, std::bind(SimulateUnloadToFINDA, _1, 500'000, 10'000), 200'000));
    REQUIRE(VerifyState(tc, mg::FilamentLoadState::InSelector, mi::idler.IdleSlotIndex(), fromSlot, false, false, ml::off, ml::blink0, ErrorCode::FSENSOR_DIDNT_SWITCH_OFF, ProgressCode::UnloadingFilament));
    REQUIRE(tc.unl.State() == ProgressCode::ERRWaitingForUser);
}

void ToolChangeFailFSensorMiddleBtn(logic::ToolChange &tc, uint8_t fromSlot, uint8_t toSlot) {
    using namespace std::placeholders;

    // user pulls filament out from the fsensor and presses Retry
    SetFSensorStateAndDebounce(false);
    REQUIRE_FALSE(mui::userInput.AnyEvent());
    PressButtonAndDebounce(tc, mb::Middle, true);

    REQUIRE(VerifyState(tc, mg::FilamentLoadState::InSelector, mi::idler.IdleSlotIndex(), fromSlot, false, false, ml::blink0, ml::off, ErrorCode::RUNNING, ProgressCode::UnloadingFilament));
    REQUIRE(tc.unl.State() == ProgressCode::FeedingToFinda); // MMU must find out where the filament is FS is OFF, FINDA is OFF

    // both movables should have their homing flag invalidated
    REQUIRE_FALSE(mi::idler.HomingValid());
    REQUIRE_FALSE(ms::selector.HomingValid());

    // make FINDA trigger - Idler will rehome in this step, Selector must remain at its place
    SimulateIdlerHoming(tc);

    REQUIRE_FALSE(mi::idler.HomingValid());
    REQUIRE_FALSE(ms::selector.HomingValid());

    SimulateIdlerWaitForHomingValid(tc);

    REQUIRE(mi::idler.HomingValid());
    REQUIRE_FALSE(ms::selector.HomingValid());

    SimulateIdlerMoveToParkingPosition(tc);

    // now trigger the FINDA
    REQUIRE(WhileCondition(tc, std::bind(SimulateFeedToFINDA, _1, 100), 5000));
    REQUIRE(VerifyState(tc, mg::FilamentLoadState::InSelector, fromSlot, fromSlot, true, true, ml::blink0, ml::off, ErrorCode::RUNNING, ProgressCode::UnloadingFilament));
    REQUIRE(tc.unl.State() == ProgressCode::RetractingFromFinda);

    // make FINDA switch off
    REQUIRE(WhileCondition(tc, std::bind(SimulateRetractFromFINDA, _1, 100), 5000));
    REQUIRE(WhileCondition(
        tc, [&](uint32_t) { return tc.unl.State() == ProgressCode::RetractingFromFinda; }, 50000));

    // Selector will start rehoming at this stage - that was the error this test was to find
    REQUIRE(VerifyState(tc, mg::FilamentLoadState::AtPulley, fromSlot, config::toolCount, false, true, ml::blink0, ml::off, ErrorCode::RUNNING, ProgressCode::UnloadingFilament));
    REQUIRE(tc.unl.State() == ProgressCode::DisengagingIdler);
    SimulateSelectorHoming(tc);

    // Idler has probably engaged meanwhile, ignore its position check
    REQUIRE(WhileTopState(tc, ProgressCode::UnloadingFilament, 50000));
    REQUIRE(VerifyState2(tc, mg::FilamentLoadState::AtPulley, config::toolCount, fromSlot, false, false, toSlot, ml::blink0, ml::off, ErrorCode::RUNNING, ProgressCode::FeedingToFinda));

    // after that, perform a normal load
    FeedingToFinda(tc, toSlot, 100);
    FeedingToBondtech(tc, toSlot);
    CheckFinishedCorrectly(tc, toSlot);
}

TEST_CASE("tool_change::load_fail_FSensor_resolve_btnM", "[tool_change]") {
    logic::ToolChange tc;
    for (uint8_t fromSlot = 0; fromSlot < config::toolCount; ++fromSlot) {
        for (uint8_t toSlot = 0; toSlot < config::toolCount; ++toSlot) {
            if (fromSlot != toSlot) {
                ToolChangeFailFSensor(tc, fromSlot, toSlot);
                ToolChangeFailFSensorMiddleBtn(tc, fromSlot, toSlot);
            }
        }
    }
}

void ToolChangeWithFlickeringFINDA(logic::ToolChange &tc, uint8_t fromSlot, uint8_t toSlot, bool keepFindaPressed) {
    ForceReinitAllAutomata();
    SetMinimalBowdenLength();

    REQUIRE(EnsureActiveSlotIndex(fromSlot, mg::FilamentLoadState::InNozzle));
    SetFINDAStateAndDebounce(true);
    SetFSensorStateAndDebounce(true);

    // restart the automaton
    tc.Reset(toSlot);

    REQUIRE(WhileCondition(tc, std::bind(SimulateUnloadToFINDA, _1, 100, 2'000), 200'000));

    // This is something else than WhileTopState()==UnloadingFilament
    // We need to catch the very moment, when the unload finished and a move to another slot is being planned
    REQUIRE(WhileCondition(
        tc, [&](uint32_t) -> bool { return tc.unl.State() != ProgressCode::OK; }, 5000));

    // now press FINDA again, but prevent stepping other state machines
    REQUIRE_FALSE(mf::finda.Pressed());
    hal::gpio::WritePin(FINDA_PIN, hal::gpio::Level::high);
    while (!mf::finda.Pressed()) {
        mf::finda.Step();
        mt::IncMillis();
    }
    REQUIRE(mf::finda.Pressed());
    REQUIRE(mg::globals.FilamentLoaded() == mg::FilamentLoadState::AtPulley);

    // now what ;) - Selector cannot move, because FINDA is pressed
    // ToolChange should emit "FINDA_DIDNT_SWITCH_OFF" and we should be able to resolve the error by Retrying
    main_loop();
    tc.Step();

    // Here is a similar problem like in ToolChangeFailLoadToFindaMiddleBtn:
    // FINDA state doesn't correspond to mg::globals.filamentLoaded, but in this case it is correct.
    // The previous unload finished correctly but FINDA flickers - that means filament tip is expected
    // to be AtPulley and not blocking the Selector.
    //
    // @@TODO however - red LED is probably not blinking on the correct slot. We are still standing on fromSlot
    // while red LED will blink on toSlot.
    // Atm this is not easy to change without some nontrivial changes to GoToErrDisengagingIdler()
    REQUIRE(VerifyState2(tc, mg::FilamentLoadState::AtPulley, mi::idler.IdleSlotIndex(), fromSlot, true, false, toSlot, ml::off, ml::blink0, ErrorCode::RUNNING, ProgressCode::ERRDisengagingIdler));
    SimulateErrDisengagingIdler(tc, ErrorCode::FINDA_FLICKERS); // this should be a single step, Idler should remain disengaged due to previous error

    // now we have 2 options what can happen:
    // FINDA is still pressed - the user didn't manage to fix the issue
    // FINDA is not pressed - the print should continue
    if (keepFindaPressed) {
        // now waiting for user input
        REQUIRE_FALSE(mui::userInput.AnyEvent());
        REQUIRE(VerifyState2(tc, mg::FilamentLoadState::AtPulley, mi::idler.IdleSlotIndex(), fromSlot, true, false, toSlot, ml::off, ml::blink0, ErrorCode::FINDA_FLICKERS, ProgressCode::ERRWaitingForUser));
        PressButtonAndDebounce(tc, mb::Middle, true);
        // we should remain in the same error state
        REQUIRE(VerifyState2(tc, mg::FilamentLoadState::AtPulley, mi::idler.IdleSlotIndex(), fromSlot, true, false, toSlot, ml::off, ml::blink0, ErrorCode::RUNNING, ProgressCode::ERRDisengagingIdler));

        // Idler would like to rehome at this spot - theoretically it is free to do so and actually will have the homing move planned.
        // In reality, one main cycle of the FW takes ~1ms so the Idler will never really move - which is exactly what we want to leverage

        // perform just one step to fall into the same error again
        main_loop();
        tc.Step();

        // now both Idler and Selector are on hold again
        REQUIRE(mi::idler.IsOnHold());
        REQUIRE(ms::selector.IsOnHold());

        REQUIRE(VerifyState2(tc, mg::FilamentLoadState::AtPulley, mi::idler.IdleSlotIndex(), fromSlot, true, false, toSlot, ml::off, ml::blink0, ErrorCode::FINDA_FLICKERS, ProgressCode::ERRWaitingForUser));

        // now "fix" FINDA and the command shall finish correctly
        SetFINDAStateAndDebounce(false);
        ToolChangeFailLoadToFindaMiddleBtn(tc, toSlot);
    } else {
        SetFINDAStateAndDebounce(false);
        ToolChangeFailLoadToFindaMiddleBtn(tc, toSlot);
    }
}

TEST_CASE("tool_change::test_flickering_FINDA", "[tool_change]") {
    for (uint8_t fromSlot = 0; fromSlot < config::toolCount; ++fromSlot) {
        for (uint8_t toSlot = 0; toSlot < config::toolCount; ++toSlot) {
            logic::ToolChange tc;
            if (fromSlot != toSlot) {
                ToolChangeWithFlickeringFINDA(tc, fromSlot, toSlot, false);
            }
        }
    }
}

TEST_CASE("tool_change::test_flickering_FINDA_keepPressed", "[tool_change]") {
    for (uint8_t fromSlot = 0; fromSlot < config::toolCount; ++fromSlot) {
        for (uint8_t toSlot = 0; toSlot < config::toolCount; ++toSlot) {
            logic::ToolChange tc;
            if (fromSlot != toSlot) {
                ToolChangeWithFlickeringFINDA(tc, fromSlot, toSlot, true);
            }
        }
    }
}

void ToolChangeFSENSOR_TOO_EARLY(logic::ToolChange &tc, uint8_t slot) {
    ForceReinitAllAutomata();
    SetMinimalBowdenLength();
    REQUIRE(EnsureActiveSlotIndex(slot, mg::FilamentLoadState::AtPulley));

    // verify filament NOT loaded
    REQUIRE(VerifyEnvironmentState(mg::FilamentLoadState::AtPulley, mi::Idler::IdleSlotIndex(), slot, false, false, ml::off, ml::off));

    // restart the automaton
    tc.Reset(slot);

    FeedingToFinda(tc, slot);

    REQUIRE_FALSE(mfs::fsensor.Pressed());
    REQUIRE(WhileCondition(
        tc,
        [&](uint32_t step) -> bool {
            if(step == mm::unitToSteps<mm::P_pos_t>(config::minimumBowdenLength) / 2){ // make FSensor trigger at the half of minimal distance
                mfs::fsensor.ProcessMessage(true);
            }
            return tc.TopLevelState() == ProgressCode::FeedingToBondtech; },
        mm::unitToSteps<mm::P_pos_t>(config::minimumBowdenLength) + 10000));

    REQUIRE(VerifyState(tc, mg::FilamentLoadState::InSelector, slot, slot, true, true, ml::off, ml::blink0, ErrorCode::RUNNING, ProgressCode::ERRDisengagingIdler));
    SimulateErrDisengagingIdler(tc, ErrorCode::FSENSOR_TOO_EARLY);
    REQUIRE(VerifyState(tc, mg::FilamentLoadState::InSelector, mi::idler.IdleSlotIndex(), slot, true, false, ml::off, ml::blink0, ErrorCode::FSENSOR_TOO_EARLY, ProgressCode::ERRWaitingForUser));

    // make AutoRetry
    PressButtonAndDebounce(tc, mb::Middle, true);
    REQUIRE(VerifyState(tc, mg::FilamentLoadState::InSelector, mi::idler.IdleSlotIndex(), slot, true, true, ml::off, ml::off, ErrorCode::RUNNING, ProgressCode::UnloadingFilament));

    SimulateIdlerHoming(tc);

    // perform regular unload, just a little short (same length as above)
    REQUIRE(WhileCondition(
        tc,
        [&](uint32_t step) -> bool {
        if(step == 20){ // on 20th step make FSensor switch off
            mfs::fsensor.ProcessMessage(false);
        } else if(step == mm::unitToSteps<mm::P_pos_t>(config::minimumBowdenLength)/2){
            FINDAOnOff(false);
        }
        return tc.unl.State() != ProgressCode::DisengagingIdler; },
        200'000UL));

    // still unloading, but Selector can start homing
    SimulateSelectorHoming(tc);
    SimulateSelectorWaitForHomingValid(tc);

    // Make sure we're still in unloading state
    REQUIRE(tc.TopLevelState() == ProgressCode::UnloadingFilament);

    // The unload filament state machine explicitly waits for (ms::selector.State() == ms::Selector::Ready)
    SimulateSelectorWaitForReadyState(tc);

    // wait for finishing of UnloadingFilament
    WhileTopState(tc, ProgressCode::UnloadingFilament, 5000);

    REQUIRE(VerifyState2(tc, mg::FilamentLoadState::AtPulley, mi::Idler::IdleSlotIndex(), slot, false, false, slot, ml::blink0, ml::off, ErrorCode::RUNNING, ProgressCode::FeedingToFinda));
    FeedingToFinda(tc, slot);
    FeedingToBondtech(tc, slot);
    CheckFinishedCorrectly(tc, slot);
}

TEST_CASE("tool_change::test_FSENSOR_TOO_EARLY", "[tool_change]") {
    for (uint8_t slot = 0; slot < config::toolCount; ++slot) {
        logic::ToolChange tc;
        ToolChangeFSENSOR_TOO_EARLY(tc, slot);
    }
}
