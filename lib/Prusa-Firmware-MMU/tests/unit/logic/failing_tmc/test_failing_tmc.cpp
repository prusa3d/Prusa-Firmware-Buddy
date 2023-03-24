// This test checks reporting errors while some logic operation is in progress
// As a base for this test, unload_filament was chosen.
// Moreover, I didn't want to spoil the unit tests of the state machines themself with this.

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

#include "../../../../src/logic/unload_filament.h"

#include "../../modules/stubs/stub_adc.h"

#include "../stubs/main_loop_stub.h"
#include "../stubs/stub_motion.h"

using Catch::Matchers::Equals;

#include "../helpers/helpers.ipp"

void CauseTMCError(mm::Axis axis, hal::tmc2130::ErrorFlags ef) {
    hal::tmc2130::TMC2130 &tmc = const_cast<hal::tmc2130::TMC2130 &>(mm::motion.DriverForAxis(axis));
    tmc.SetErrorFlags(ef);
}

inline ErrorCode operator|(ErrorCode a, ErrorCode b) {
    return (ErrorCode)((uint16_t)a | (uint16_t)b);
}

void FailingIdler(hal::tmc2130::ErrorFlags ef, ErrorCode ec) {
    // prepare startup conditions
    ForceReinitAllAutomata();

    // change the startup to what we need here
    EnsureActiveSlotIndex(0, mg::FilamentLoadState::InNozzle);

    // set FINDA ON + debounce
    SetFINDAStateAndDebounce(true);

    logic::UnloadFilament uf;

    // verify startup conditions
    REQUIRE(VerifyState(uf, mg::FilamentLoadState::InNozzle, mi::Idler::IdleSlotIndex(), 0, true, false, ml::off, ml::off, ErrorCode::OK, ProgressCode::OK));

    // UnloadFilament starts by engaging the idler (through the UnloadToFinda state machine)
    uf.Reset(0);

    REQUIRE(VerifyState(uf, mg::FilamentLoadState::InNozzle, mi::Idler::IdleSlotIndex(), 0, true, true, ml::off, ml::off, ErrorCode::RUNNING, ProgressCode::UnloadingToFinda));

    uint32_t failingStep = 5;
    REQUIRE(WhileCondition(
        uf,
        [&](uint32_t step) -> bool {
        if(step == failingStep){ // on 5th step make the TMC report some error
            CauseTMCError(mm::Idler, ef);
        }
        return uf.TopLevelState() == ProgressCode::UnloadingToFinda; },
        5000));

    // the simulated motion may proceed, but I don't care here. In reality no one really knows what the TMC does
    // The checked value is not really important here (just that it moves!), so with tuning of the constants it may break the unit test
    // Therefore it is disabled by default
    // REQUIRE(mm::axes[mm::Idler].pos == failingStep * config::pulleyToCuttingEdge.v + 1);

    // repeated calls to step this logic automaton shall produce no change
    for (int i = 0; i < 5; ++i) {
        // and this must cause the state machine to run into a TMC error state and report the error correctly
        // Please note we are leaving the Idler in an intermediate position due to the TMC failure,
        // so we cannot use the usual VerifyState(), but have to check the stuff manually
        // REQUIRE(VerifyState(uf, true, raw_6, 0, false, ml::blink0, ml::blink0, ec, ProgressCode::ERRTMCFailed));
        REQUIRE(ml::leds.Mode(0, ml::red) == ml::off);
        REQUIRE(ml::leds.Mode(0, ml::green) == ml::blink0);
        REQUIRE(uf.Error() == ec);
        REQUIRE(uf.TopLevelState() == ProgressCode::ERRTMCFailed);

        main_loop();
        uf.Step();
    }
}

TEST_CASE("failing_tmc::failing_idler", "[failing_tmc]") {
    hal::tmc2130::ErrorFlags ef;
    ef.ot = 1; // make the TMC hot like hell
    FailingIdler(ef, ErrorCode::TMC_OVER_TEMPERATURE_ERROR | ErrorCode::TMC_IDLER_BIT);
}
