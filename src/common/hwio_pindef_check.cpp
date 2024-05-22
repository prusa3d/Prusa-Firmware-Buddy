/**
 * @file
 * @date Sep 19, 2020
 * @author Marek Bel
 */

#include "hwio_pindef.h"
#include <iterator>

using buddy::hw::PinChecker;

constexpr PinChecker pinsToCheck[] = {
    // check physical pins
    PIN_TABLE(PINS_TO_CHECK)
    // and also check virtual pins
    VIRTUAL_PIN_TABLE(VIRTUAL_PINS_TO_CHECK)
};

constexpr bool has_duplicates(const PinChecker (&array)[std::size(pinsToCheck)]) {
    for (size_t i = 1; i < std::size(array); i++) {
        for (size_t j = 0; j < i; j++) {
            if ((array[i].getPort() == array[j].getPort()) && (array[i].getPin() == array[j].getPin())) {
                return true;
            }
        }
    }
    return false;
}

static_assert(!has_duplicates(pinsToCheck), "Some physical pin defined under two names.");

#include <device/hal.h>

static constexpr bool is_valid_port(buddy::hw::IoPort ioPort, [[maybe_unused]] buddy::hw::IoPin ioPin) {
    switch (ioPort) {
    case buddy::hw::IoPort::A:
    case buddy::hw::IoPort::B:
    case buddy::hw::IoPort::C:
    case buddy::hw::IoPort::D:
    case buddy::hw::IoPort::E:
    case buddy::hw::IoPort::F:
#ifdef GPIOG_BASE
    case buddy::hw::IoPort::G:
#endif
        return true;
    default:
        return false;
    }
}

static constexpr bool is_valid_pin([[maybe_unused]] buddy::hw::IoPort ioPort, buddy::hw::IoPin ioPin) {
    switch (ioPin) {
    case buddy::hw::IoPin::p0:
    case buddy::hw::IoPin::p1:
    case buddy::hw::IoPin::p2:
    case buddy::hw::IoPin::p3:
    case buddy::hw::IoPin::p4:
    case buddy::hw::IoPin::p5:
    case buddy::hw::IoPin::p6:
    case buddy::hw::IoPin::p7:
    case buddy::hw::IoPin::p8:
    case buddy::hw::IoPin::p9:
    case buddy::hw::IoPin::p10:
    case buddy::hw::IoPin::p11:
    case buddy::hw::IoPin::p12:
    case buddy::hw::IoPin::p13:
    case buddy::hw::IoPin::p14:
    case buddy::hw::IoPin::p15:
        return true;
    default:
        return false;
    }
}

#define PORTS_TO_VALIDATE(TYPE, NAME, PORTPIN, PARAMETERS, INTERRUPT_HANDLER) static_assert(is_valid_port(PORTPIN), "Invalid port number.");
#define PINS_TO_VALIDATE(TYPE, NAME, PORTPIN, PARAMETERS, INTERRUPT_HANDLER)  static_assert(is_valid_pin(PORTPIN), "Invalid pin number.");

PIN_TABLE(PORTS_TO_VALIDATE)
PIN_TABLE(PINS_TO_VALIDATE)
