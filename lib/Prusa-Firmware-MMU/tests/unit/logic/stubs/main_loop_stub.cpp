#include "main_loop_stub.h"

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
#include "../../../../src/modules/selector.h"
#include "../../../../src/modules/user_input.h"

#include "../stubs/stub_motion.h"

#include <new> // bring in placement new
#include <stddef.h>

void main_loop() {
    mb::buttons.Step();
    ml::leds.Step();
    mf::finda.Step();
    mfs::fsensor.Step();
    mi::idler.Step();
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
    new (&ms::selector) ms::Selector();
    new (&mm::motion) mm::Motion();

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
    mg::globals.SetFilamentLoaded(mg::globals.ActiveSlot(), mg::FilamentLoadState::AtPulley);
}

void HomeIdlerAndSelector() {
    ms::selector.InvalidateHoming();
    mi::idler.InvalidateHoming();
    SimulateIdlerAndSelectorHoming();
}

void SimulateIdlerAndSelectorHoming() {
    // do 5 steps until we trigger the simulated stallguard
    for (uint8_t i = 0; i < 5; ++i) {
        main_loop();
    }

    mm::TriggerStallGuard(mm::Selector);
    mm::TriggerStallGuard(mm::Idler);

    // now the Selector and Idler shall perform a move into their parking positions
    while (ms::selector.State() != mm::MovableBase::Ready || mi::idler.State() != mm::MovableBase::Ready)
        main_loop();

    mm::motion.StallGuardReset(mm::Selector);
    mm::motion.StallGuardReset(mm::Idler);
}

void SimulateIdlerHoming() {
    // do 5 steps until we trigger the simulated stallguard
    for (uint8_t i = 0; i < 5; ++i) {
        main_loop();
    }

    mm::TriggerStallGuard(mm::Idler);

    // now the Idler shall perform a move into their parking positions
    while (mi::idler.State() != mm::MovableBase::Ready)
        main_loop();

    mm::motion.StallGuardReset(mm::Idler);
}

void SimulateSelectorHoming() {
    // do 5 steps until we trigger the simulated stallguard
    for (uint8_t i = 0; i < 5; ++i) {
        main_loop();
    }

    mm::TriggerStallGuard(mm::Selector);

    // now the Selector shall perform a move into their parking positions
    while (ms::selector.State() != mm::MovableBase::Ready)
        main_loop();

    mm::motion.StallGuardReset(mm::Selector);
}

void EnsureActiveSlotIndex(uint8_t slot, mg::FilamentLoadState loadState) {
    HomeIdlerAndSelector();

    // move selector to the right spot
    ms::selector.MoveToSlot(slot);
    while (ms::selector.Slot() != slot)
        main_loop();

    // mg::globals.SetActiveSlot(slot);
    mg::globals.SetFilamentLoaded(slot, loadState);
}

void SetFINDAStateAndDebounce(bool press) {
    hal::gpio::WritePin(FINDA_PIN, hal::gpio::Level::high);
    for (size_t i = 0; i < config::findaDebounceMs + 1; ++i)
        main_loop();
}
