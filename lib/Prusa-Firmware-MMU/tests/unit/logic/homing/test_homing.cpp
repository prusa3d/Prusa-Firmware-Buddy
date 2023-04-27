#include "catch2/catch_test_macros.hpp"
#include "catch2/matchers/catch_matchers_vector.hpp"

#include "../../../../src/modules/buttons.h"
#include "../../../../src/modules/finda.h"
#include "../../../../src/modules/fsensor.h"
#include "../../../../src/modules/globals.h"
#include "../../../../src/modules/idler.h"
#include "../../../../src/modules/leds.h"
#include "../../../../src/modules/motion.h"
#include "../../../../src/modules/permanent_storage.h"
#include "../../../../src/modules/selector.h"

#include "../../../../src/logic/home.h"
#include "../../../../src/logic/load_filament.h"
#include "../../../../src/logic/unload_filament.h"

#include "../../modules/stubs/stub_adc.h"

#include "../stubs/homing.h"
#include "../stubs/main_loop_stub.h"
#include "../stubs/stub_motion.h"

using Catch::Matchers::Equals;

#include "../helpers/helpers.ipp"

bool SuccessfulHome(uint8_t slot) {
    // prepare startup conditions
    ForceReinitAllAutomata();

    // change the startup to what we need here
    REQUIRE(EnsureActiveSlotIndex(slot, mg::FilamentLoadState::AtPulley));

    // set FINDA OFF + debounce
    SetFINDAStateAndDebounce(false);

    logic::Home h;
    REQUIRE(VerifyState(h, mg::FilamentLoadState::AtPulley, mi::Idler::IdleSlotIndex(), slot, false, false, ml::off, ml::off, ErrorCode::OK, ProgressCode::OK));

    h.Reset(0);
    REQUIRE(VerifyState(h, mg::FilamentLoadState::AtPulley, mi::Idler::IdleSlotIndex(), slot, false, false, ml::off, ml::off, ErrorCode::RUNNING, ProgressCode::Homing));
    REQUIRE_FALSE(mi::idler.HomingValid());
    REQUIRE_FALSE(ms::selector.HomingValid());

    SimulateIdlerAndSelectorHoming(h);

    REQUIRE(WhileTopState(h, ProgressCode::Homing, 5000));

    REQUIRE(VerifyState(h, mg::FilamentLoadState::AtPulley, mi::Idler::IdleSlotIndex(), slot, false, false, ml::off, ml::off, ErrorCode::OK, ProgressCode::OK));
    REQUIRE(mi::idler.HomingValid());
    REQUIRE(ms::selector.HomingValid());

    return true;
}

TEST_CASE("homing::successful_run", "[homing]") {
    for (uint8_t slot = 0; slot < config::toolCount; ++slot) {
        REQUIRE(SuccessfulHome(slot));
    }
}

bool SelectorFailedRetry() {
    // prepare startup conditions
    ForceReinitAllAutomata();

    // change the startup to what we need here
    REQUIRE(EnsureActiveSlotIndex(0, mg::FilamentLoadState::AtPulley));

    // set FINDA OFF + debounce
    SetFINDAStateAndDebounce(false);

    logic::Home h;
    REQUIRE(VerifyState(h, mg::FilamentLoadState::AtPulley, mi::Idler::IdleSlotIndex(), 0, false, false, ml::off, ml::off, ErrorCode::OK, ProgressCode::OK));

    h.Reset(0);
    REQUIRE(VerifyState(h, mg::FilamentLoadState::AtPulley, mi::Idler::IdleSlotIndex(), 0, false, false, ml::off, ml::off, ErrorCode::RUNNING, ProgressCode::Homing));

    REQUIRE(SimulateFailedHomeFirstTime(h));

    for (uint8_t i = 0; i < 5; ++i) {
        REQUIRE(SimulateFailedHomeSelectorRepeated(h));
    }

    SimulateSelectorHoming(h);

    REQUIRE(WhileTopState(h, ProgressCode::Homing, 5000));

    REQUIRE(VerifyState(h, mg::FilamentLoadState::AtPulley, mi::Idler::IdleSlotIndex(), 0, false, false, ml::off, ml::off, ErrorCode::OK, ProgressCode::OK));
    REQUIRE(mi::idler.HomingValid());
    REQUIRE(ms::selector.HomingValid());

    return true;
}

TEST_CASE("homing::selector_failed_retry", "[homing]") {
    REQUIRE(SelectorFailedRetry());
}

bool RefusedMove(uint8_t slot) {
    // prepare startup conditions
    ForceReinitAllAutomata();

    // change the startup to what we need here
    HomeIdlerAndSelector();

    SetFINDAStateAndDebounce(true);
    mg::globals.SetFilamentLoaded(slot, mg::FilamentLoadState::InSelector);

    // move selector to the right spot - should not be possible
    REQUIRE(ms::selector.MoveToSlot(slot) == ms::Selector::OperationResult::Refused);
    return true;
}

bool RefusedHome(uint8_t slot) {
    // prepare startup conditions
    ForceReinitAllAutomata();

    // change the startup to what we need here
    HomeIdlerAndSelector();

    SetFINDAStateAndDebounce(true);
    mg::globals.SetFilamentLoaded(slot, mg::FilamentLoadState::InSelector);

    ms::selector.InvalidateHoming();

    // selector should not start homing, because something is in the FINDA
    for (uint8_t i = 0; i < 100; ++i) {
        main_loop();
        REQUIRE_FALSE(ms::selector.HomingValid());
        REQUIRE(ms::selector.State() == ms::Selector::Ready);
    }
    // unpress FINDA
    SetFINDAStateAndDebounce(false);
    mg::globals.SetFilamentLoaded(slot, mg::FilamentLoadState::AtPulley);

    // selector should start the homing sequence
    main_loop(); // plans the homing move
    // since the Idler is ok, the Selector should start homing immediately
    main_loop();
    REQUIRE(ms::selector.State() == ms::Selector::HomeForward);
    return true;
}

TEST_CASE("homing::refused_move", "[homing]") {
    for (uint8_t slot = 0; slot < config::toolCount; ++slot) {
        REQUIRE(RefusedMove(slot));
    }
}

TEST_CASE("homing::refused_home", "[homing]") {
    for (uint8_t slot = 0; slot < config::toolCount; ++slot) {
        REQUIRE(RefusedHome(slot));
    }
}

bool OnHold(uint8_t slot) {
    // prepare startup conditions
    ForceReinitAllAutomata();

    // change the startup to what we need here
    HomeIdlerAndSelector();

    SetFINDAStateAndDebounce(true);
    mg::globals.SetFilamentLoaded(slot, mg::FilamentLoadState::InSelector);

    // now put movables on hold
    logic::CommandBase::HoldIdlerSelector();

    REQUIRE(mi::idler.state == mi::Idler::OnHold);
    REQUIRE(ms::selector.state == ms::Selector::OnHold);

    // both movables should ignore all attempts to perform moves
    REQUIRE(mi::idler.PlanHome() == mi::Idler::OperationResult::Refused);
    REQUIRE(mi::idler.state == mi::Idler::OnHold);
    REQUIRE(mi::idler.Disengaged());

    REQUIRE(ms::selector.PlanHome() == ms::Selector::OperationResult::Refused);
    REQUIRE(ms::selector.state == ms::Selector::OnHold);

    REQUIRE(mi::idler.Disengage() == mi::Idler::OperationResult::Refused);
    REQUIRE(mi::idler.state == mi::Idler::OnHold);
    REQUIRE(mi::idler.Engage(slot) == mi::Idler::OperationResult::Refused);
    REQUIRE(mi::idler.state == mi::Idler::OnHold);
    REQUIRE(mi::idler.Disengaged());

    REQUIRE(ms::selector.MoveToSlot((slot + 1) % config::toolCount) == ms::Selector::OperationResult::Refused);
    REQUIRE(ms::selector.state == ms::Selector::OnHold);

    return true;
}

TEST_CASE("homing::on-hold", "[homing]") {
    for (uint8_t slot = 0; slot < config::toolCount; ++slot) {
        REQUIRE(OnHold(slot));
    }
}
