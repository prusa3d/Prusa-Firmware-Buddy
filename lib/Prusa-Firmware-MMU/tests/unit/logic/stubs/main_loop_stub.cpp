#include "catch2/catch_test_macros.hpp"
#include "main_loop_stub.h"
#include "homing.h"

#include "../../modules/stubs/stub_adc.h"
#include "../../modules/stubs/stub_eeprom.h"
#include "../../modules/stubs/stub_timebase.h"

#include "../../../../src/modules/buttons.h"
#include "../../../../src/modules/finda.h"
#include "../../../../src/modules/fsensor.h"
#include "../../../../src/modules/globals.h"
#include "../../../../src/modules/idler.h"
#include "../../../../src/modules/leds.h"
#include "../../../../src/modules/motion.h"
#include "../../../../src/modules/permanent_storage.h"
#include "../../../../src/modules/pulley.h"
#include "../../../../src/modules/selector.h"
#include "../../../../src/modules/user_input.h"

#include "../../../../src/logic/no_command.h"

#include "../stubs/stub_motion.h"

#include <new> // bring in placement new
#include <stddef.h>

void main_loop() {
    mb::buttons.Step();
    ml::leds.Step();
    mf::finda.Step();
    mfs::fsensor.Step();
    mi::idler.Step();
    mpu::pulley.Step();
    ms::selector.Step();
    mm::motion.Step();
    mui::userInput.Step();

    mt::IncMillis();
}

void ForceReinitAllAutomata() {
    // This woodoo magic with placement new is just a forced reinit of global instances of firmware's state machines
    // just for the purposes of separate unit tests. Each unit test needs a "freshly booted firmware" and since all unit tests
    // in the test binary share the same global data structures, we need some way of making them fresh each time.
    //
    // This approach mimics the firmware behavior closely as the firmware initializes its global data structures
    // on its very start once (by copying static init data from PROGMEM into RAM) - and we need exactly this approach in the unit tests.
    //
    // There are multiple other approaches, one of them is adding a special Init() function into each of these state machines.
    // As this approach might look like a standard and safer way of doing stuff, it has several drawbacks, especially
    // it needs an explicit call to the Init function every time an object like this is created - this can have negative influence on firmware's code size

    new (&mb::buttons) mb::Buttons();
    new (&ml::leds) ml::LEDs();
    new (&mf::finda) mf::FINDA();
    new (&mfs::fsensor) mfs::FSensor();
    new (&mi::idler) mi::Idler();
    new (&mpu::pulley) mpu::Pulley();
    new (&ms::selector) ms::Selector();
    new (&mm::motion) mm::Motion();
    new (&mui::userInput) mui::UserInput();

    hal::eeprom::ClearEEPROM();

    // no buttons involved ;)
    hal::adc::ReinitADC(config::buttonsADCIndex, hal::adc::TADCData({ 1023 }), 1);

    // finda OFF
    hal::gpio::WritePin(FINDA_PIN, hal::gpio::Level::low);

    // reinit timing
    mt::ReinitTimebase();

    // reinit axes positions
    mm::ReinitMotion();

    // let's assume we have the filament NOT loaded and active slot 0
    mg::globals.Init();
    mg::globals.SetFilamentLoaded(mg::globals.ActiveSlot(), mg::FilamentLoadState::AtPulley);
}

void HomeIdlerAndSelector() {
    mi::idler.InvalidateHoming();
    ms::selector.InvalidateHoming();
    logic::NoCommand nc; // just a dummy instance which has an empty Step()

    SimulateIdlerHoming(nc);
    SimulateIdlerWaitForHomingValid(nc);
    SimulateIdlerMoveToParkingPosition(nc);

    SimulateSelectorHoming(nc);
    SimulateSelectorWaitForHomingValid(nc);
    SimulateSelectorWaitForReadyState(nc);
}

bool EnsureActiveSlotIndex(uint8_t slot, mg::FilamentLoadState loadState) {
    HomeIdlerAndSelector();

    // move selector to the right spot
    if (ms::selector.MoveToSlot(slot) == ms::Selector::OperationResult::Refused)
        return false;
    while (ms::selector.Slot() != slot)
        main_loop();

    // mg::globals.SetActiveSlot(slot);
    mg::globals.SetFilamentLoaded(slot, loadState);
    return true;
}

void SetFINDAStateAndDebounce(bool press) {
    hal::gpio::WritePin(FINDA_PIN, press ? hal::gpio::Level::high : hal::gpio::Level::low);
    for (size_t i = 0; i < config::findaDebounceMs + 1; ++i)
        main_loop();
}

// The idea is to set fsOff and findaOff to some reasonable values (like 10 and 1000)
// for normal situations.
// For errorneous situations set fsOff or findaOff to some number higher than the number of steps
// the testing routine is allowed to do -> thus effectively blocking the corresponding moment for fsensor
// and finda switching off
bool SimulateUnloadToFINDA(uint32_t step, uint32_t fsOff, uint32_t findaOff) {
    if (step == fsOff) { // make FSensor switch off
        mfs::fsensor.ProcessMessage(false);
        return true;
    } else if (step == findaOff) { // make FINDA switch off
        hal::gpio::WritePin(FINDA_PIN, hal::gpio::Level::low);
    }
    return mf::finda.Pressed();
}

bool SimulateFeedToFINDA(uint32_t step, uint32_t findaOn) {
    if (step == findaOn) {
        hal::gpio::WritePin(FINDA_PIN, hal::gpio::Level::high);
    }
    return !mf::finda.Pressed();
}

bool SimulateRetractFromFINDA(uint32_t step, uint32_t findaOff) {
    if (step == findaOff) {
        hal::gpio::WritePin(FINDA_PIN, hal::gpio::Level::low);
    }
    return mf::finda.Pressed();
}

void PressButtonAndDebounce(logic::CommandBase &cb, uint8_t btnIndex, bool fromPrinter) {
    if (fromPrinter) {
        mui::userInput.ProcessMessage(btnIndex);
        main_loop();
        cb.Step();
    } else {
        hal::adc::SetADC(config::buttonsADCIndex, config::buttonADCLimits[btnIndex][0] + 1);
        while (!mb::buttons.ButtonPressed(btnIndex)) {
            main_loop();
            cb.Step(); // Inner
        }
    }
}

void ClearButtons(logic::CommandBase &cb) {
    hal::adc::SetADC(config::buttonsADCIndex, config::buttonADCMaxValue);
    while (mb::buttons.AnyButtonPressed()) {
        main_loop();
        cb.Step(); // Inner
    }
}

void SetMinimalBowdenLength() {
    // reset bowdenLenght in EEPROM
    mps::BowdenLength::Set(config::minimumBowdenLength.v);
}

void SetFSensorStateAndDebounce(bool press) {
    mfs::fsensor.ProcessMessage(press);
    for (uint8_t fs = 0; fs < config::fsensorDebounceMs + 1; ++fs) {
        main_loop();
    }
}

void SimulateErrDisengagingIdler(logic::CommandBase &cb, ErrorCode deferredEC) {
    REQUIRE(WhileCondition(
        cb, [&](uint32_t) {
            if (cb.TopLevelState() == ProgressCode::ERRDisengagingIdler) {
                REQUIRE(cb.Error() == ErrorCode::RUNNING); // ensure the error gets never set while disengaging the idler
                return true;
            } else {
                REQUIRE(cb.Error() == deferredEC);
                return false;
            }
        },
        idlerEngageDisengageMaxSteps));
}
