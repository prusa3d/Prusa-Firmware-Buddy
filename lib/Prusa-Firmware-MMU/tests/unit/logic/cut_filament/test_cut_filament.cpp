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

#include "../../../../src/logic/cut_filament.h"

#include "../../modules/stubs/stub_adc.h"

#include "../stubs/homing.h"
#include "../stubs/main_loop_stub.h"
#include "../stubs/stub_motion.h"

#include "../helpers/helpers.ipp"

void CutSlot(logic::CutFilament &cf, uint8_t startSlot, uint8_t cutSlot) {

    ForceReinitAllAutomata();
    REQUIRE(EnsureActiveSlotIndex(startSlot, mg::FilamentLoadState::AtPulley));

    REQUIRE(VerifyState(cf, mg::FilamentLoadState::AtPulley, mi::Idler::IdleSlotIndex(), startSlot, false, false, ml::off, ml::off, ErrorCode::OK, ProgressCode::OK));

    // restart the automaton
    cf.Reset(cutSlot);

    // check initial conditions
    REQUIRE(VerifyState2(cf, mg::FilamentLoadState::AtPulley, mi::Idler::IdleSlotIndex(), startSlot, false, false, cutSlot, ml::blink0, ml::off, ErrorCode::RUNNING, ProgressCode::SelectingFilamentSlot));

    // now cycle at most some number of cycles (to be determined yet) and then verify, that the idler and selector reached their target positions
    // Beware - with the real positions of the selector, the number of steps needed to finish some states grows, so the ~40K steps here has a reason
    REQUIRE(WhileTopState(cf, ProgressCode::SelectingFilamentSlot, selectorMoveMaxSteps));

    // idler and selector reached their target positions and the CF automaton will start feeding to FINDA as the next step
    REQUIRE(VerifyState2(cf, mg::FilamentLoadState::AtPulley, cutSlot, cutSlot, false, false, cutSlot, ml::blink0, ml::off, ErrorCode::RUNNING, ProgressCode::FeedingToFinda));

    // prepare for simulated finda trigger
    REQUIRE(WhileCondition(
        cf,
        [&](uint32_t step) -> bool {
        if( step == 100 ){ // simulate FINDA trigger - will get pressed in 100 steps (due to debouncing)
            hal::gpio::WritePin(FINDA_PIN, hal::gpio::Level::high);
        }
        return cf.TopLevelState() == ProgressCode::FeedingToFinda; }, 5000));

    // filament fed to FINDA
    REQUIRE(VerifyState2(cf, mg::FilamentLoadState::InSelector, cutSlot, cutSlot, true, true, cutSlot, ml::blink0, ml::off, ErrorCode::RUNNING, ProgressCode::RetractingFromFinda));

    // pull it back to the pulley + simulate FINDA depress
    REQUIRE(WhileCondition(
        cf,
        [&](uint32_t step) -> bool {
        if( step == 100 ){ // simulate FINDA trigger - will get depressed in 100 steps
            hal::gpio::WritePin(FINDA_PIN, hal::gpio::Level::low);
        }
        return cf.TopLevelState() == ProgressCode::RetractingFromFinda; }, 5000));

    REQUIRE(VerifyState2(cf, mg::FilamentLoadState::AtPulley, cutSlot, cutSlot, false, true, cutSlot, ml::blink0, ml::off, ErrorCode::RUNNING, ProgressCode::PreparingBlade));

    // now move the selector aside, prepare for cutting
    REQUIRE(WhileTopState(cf, ProgressCode::PreparingBlade, 5000));
    REQUIRE(VerifyState2(cf, mg::FilamentLoadState::AtPulley, cutSlot, cutSlot + 1, false, true, cutSlot, ml::blink0, ml::off, ErrorCode::RUNNING, ProgressCode::PushingFilament));

    // pushing filament a bit for a cut
    REQUIRE(WhileTopState(cf, ProgressCode::PushingFilament, 5000));
    REQUIRE(VerifyState2(cf, mg::FilamentLoadState::AtPulley, cutSlot, cutSlot + 1, false, true, cutSlot, ml::blink0, ml::off, ErrorCode::RUNNING, ProgressCode::DisengagingIdler));
    // disengage idler (to save power for the selector)
    REQUIRE(WhileTopState(cf, ProgressCode::DisengagingIdler, idlerEngageDisengageMaxSteps));
    REQUIRE(VerifyState2(cf, mg::FilamentLoadState::AtPulley, mi::idler.IdleSlotIndex(), cutSlot + 1, false, true, cutSlot, ml::blink0, ml::off, ErrorCode::RUNNING, ProgressCode::PerformingCut));

    // cutting
    REQUIRE(WhileTopState(cf, ProgressCode::PerformingCut, selectorMoveMaxSteps));
    REQUIRE(VerifyState2(cf, mg::FilamentLoadState::AtPulley, mi::idler.IdleSlotIndex(), cutSlot, false, true, cutSlot, ml::blink0, ml::off, ErrorCode::RUNNING, ProgressCode::Homing));

    // rehoming selector
    SimulateSelectorHoming(cf);
    REQUIRE(WhileTopState(cf, ProgressCode::Homing, selectorMoveMaxSteps));

    // Return selector to parked position
    REQUIRE(VerifyState2(cf, mg::FilamentLoadState::AtPulley, mi::idler.IdleSlotIndex(), ms::Selector::IdleSlotIndex(), false, true, cutSlot, ml::blink0, ml::off, ErrorCode::RUNNING, ProgressCode::ReturningSelector));

    REQUIRE(WhileTopState(cf, ProgressCode::ReturningSelector, selectorMoveMaxSteps));
    REQUIRE(VerifyState2(cf, mg::FilamentLoadState::AtPulley, mi::idler.IdleSlotIndex(), cutSlot, false, true, cutSlot, ml::on, ml::off, ErrorCode::OK, ProgressCode::OK));
}

TEST_CASE("cut_filament::cut0", "[cut_filament]") {
    for (uint8_t startSlot = 0; startSlot < config::toolCount; ++startSlot) {
        for (uint8_t cutSlot = 0; cutSlot < config::toolCount; ++cutSlot) {
            logic::CutFilament cf;
            CutSlot(cf, startSlot, cutSlot);
        }
    }
}

TEST_CASE("cut_filament::invalid_slot", "[cut_filament]") {
    for (uint8_t activeSlot = 0; activeSlot < config::toolCount; ++activeSlot) {
        logic::CutFilament cf;
        InvalidSlot<logic::CutFilament>(cf, activeSlot, config::toolCount);
    }
}

TEST_CASE("cut_filament::state_machine_reusal", "[cut_filament]") {
    logic::CutFilament cf;
    for (uint8_t activeSlot = 0; activeSlot < config::toolCount; ++activeSlot) {
        InvalidSlot<logic::CutFilament>(cf, activeSlot, config::toolCount);
        // @@TODO if the automaton end up in an INVALID_TOOL error,
        // we don't get out of it by any other means than calling a Reset
        // May be that's an issue/feature of all logic layer state machines.
        // Generally not that important, but it would be nice to handle such a scenario gracefully
        cf.error = ErrorCode::OK;
    }

    for (uint8_t startSlot = 0; startSlot < config::toolCount; ++startSlot) {
        for (uint8_t cutSlot = 0; cutSlot < config::toolCount; ++cutSlot) {
            CutSlot(cf, startSlot, cutSlot);
        }
    }

    for (uint8_t startSlot = 0; startSlot < config::toolCount; ++startSlot) {
        for (uint8_t cutSlot = 0; cutSlot < config::toolCount; ++cutSlot) {
            CutSlot(cf, startSlot, cutSlot);
            InvalidSlot<logic::CutFilament>(cf, cutSlot, config::toolCount);
            cf.error = ErrorCode::OK;
        }
    }
}
