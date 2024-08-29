//----------------------------------------------------------------------------//
// pin names
#pragma once

#include "PCA9557.hpp"
#include "TCA6408A.hpp"
#include <stdint.h>
#include <device/board.h>
#include "printers.h"
#include "MarlinPin.h"
#include "buddy/priorities_config.h"

#define MARLIN_PORT_X_DIR    MARLIN_PORT_D
#define MARLIN_PIN_NR_X_DIR  MARLIN_PIN_NR_6
#define MARLIN_PORT_X_ENA    MARLIN_PORT_B // XY enable
#define MARLIN_PIN_NR_X_ENA  MARLIN_PIN_NR_9 // XY enable
#define MARLIN_PORT_X_DIAG   MARLIN_PORT_G
#define MARLIN_PIN_NR_X_DIAG MARLIN_PIN_NR_9
#define MARLIN_PORT_CS_X     MARLIN_PORT_G
#define MARLIN_PIN_NR_CS_X   MARLIN_PIN_NR_15

#define MARLIN_PORT_Y_DIR    MARLIN_PORT_D
#define MARLIN_PIN_NR_Y_DIR  MARLIN_PIN_NR_4
#define MARLIN_PORT_Y_ENA    MARLIN_PORT_B // XY enable
#define MARLIN_PIN_NR_Y_ENA  MARLIN_PIN_NR_9 // XY enable
#define MARLIN_PORT_Y_DIAG   MARLIN_PORT_E
#define MARLIN_PIN_NR_Y_DIAG MARLIN_PIN_NR_13
#define MARLIN_PORT_CS_Y     MARLIN_PORT_B
#define MARLIN_PIN_NR_CS_Y   MARLIN_PIN_NR_5

#define MARLIN_PORT_Z_STEP   MARLIN_PORT_D
#define MARLIN_PIN_NR_Z_STEP MARLIN_PIN_NR_3
#define MARLIN_PORT_Z_DIR    MARLIN_PORT_D
#define MARLIN_PIN_NR_Z_DIR  MARLIN_PIN_NR_2
#define MARLIN_PORT_Z_ENA    MARLIN_PORT_B
#define MARLIN_PIN_NR_Z_ENA  MARLIN_PIN_NR_8
#define MARLIN_PORT_Z_DIAG   MARLIN_PORT_B
#define MARLIN_PIN_NR_Z_DIAG MARLIN_PIN_NR_4
#define MARLIN_PORT_CS_Z     MARLIN_PORT_G
#define MARLIN_PIN_NR_CS_Z   MARLIN_PIN_NR_10

#define MARLIN_PORT_E0_STEP   MARLIN_PORT_D
#define MARLIN_PIN_NR_E0_STEP MARLIN_PIN_NR_1
#define MARLIN_PORT_E0_DIR    MARLIN_PORT_D
#define MARLIN_PIN_NR_E0_DIR  MARLIN_PIN_NR_0

// virtual pins, that are redirected to dwarf/modualr bed
#define MARLIN_PORT_Z_MIN   MARLIN_PORT_V // Virtual pin
#define MARLIN_PIN_NR_Z_MIN MARLIN_PIN_NR_1

#define MARLIN_PORT_XY_PROBE   MARLIN_PORT_V
#define MARLIN_PIN_NR_XY_PROBE MARLIN_PIN_NR_2

#define MARLIN_PORT_E0_ENA   MARLIN_PORT_V
#define MARLIN_PIN_NR_E0_ENA MARLIN_PIN_NR_3

#define MARLIN_PORT_E1_ENA   MARLIN_PORT_V
#define MARLIN_PIN_NR_E1_ENA MARLIN_PIN_NR_4

#define MARLIN_PORT_E2_ENA   MARLIN_PORT_V
#define MARLIN_PIN_NR_E2_ENA MARLIN_PIN_NR_5

#define MARLIN_PORT_E3_ENA   MARLIN_PORT_V
#define MARLIN_PIN_NR_E3_ENA MARLIN_PIN_NR_6

#define MARLIN_PORT_E4_ENA   MARLIN_PORT_V
#define MARLIN_PIN_NR_E4_ENA MARLIN_PIN_NR_7

#define MARLIN_PORT_E5_ENA   MARLIN_PORT_V
#define MARLIN_PIN_NR_E5_ENA MARLIN_PIN_NR_8

#define MARLIN_PORT_FAN   MARLIN_PORT_V
#define MARLIN_PIN_NR_FAN MARLIN_PIN_NR_9

#define MARLIN_PORT_FAN1   MARLIN_PORT_V
#define MARLIN_PIN_NR_FAN1 MARLIN_PIN_NR_10

#define MARLIN_PORT_THERM2   MARLIN_PORT_V
#define MARLIN_PIN_NR_THERM2 MARLIN_PIN_NR_11

#define MARLIN_PORT_X_STEP   MARLIN_PORT_V // X_STEP is virtual pin, because it is mapped to diffent pins on different HW revisions :(
#define MARLIN_PIN_NR_X_STEP MARLIN_PIN_NR_12

#define MARLIN_PORT_Y_STEP   MARLIN_PORT_V // Y_STEP is virtual pin, because it is mapped to diffent pins on different HW revisions :(
#define MARLIN_PIN_NR_Y_STEP MARLIN_PIN_NR_13

// Ambient temperature sensor connector on sandwich. Measured by AdcGet::ambientTemp().
#define MARLIN_PORT_AMBIENT   MARLIN_PORT_V
#define MARLIN_PIN_NR_AMBIENT MARLIN_PIN_NR_14

// only DUMMY pins below, to make compiler happy, but not really used

// dummy pin, we map everything that is not needed here
#define MARLIN_PORT_DUMMY   MARLIN_PORT_V
#define MARLIN_PIN_NR_DUMMY MARLIN_PIN_NR_15

#define MARLIN_PORT_CS_E   MARLIN_PORT_DUMMY
#define MARLIN_PIN_NR_CS_E MARLIN_PIN_NR_DUMMY

#define MARLIN_PORT_BED_HEAT   MARLIN_PORT_DUMMY
#define MARLIN_PIN_NR_BED_HEAT MARLIN_PIN_NR_DUMMY

#define MARLIN_PORT_TEMP_BED   MARLIN_PORT_DUMMY
#define MARLIN_PIN_NR_TEMP_BED MARLIN_PIN_NR_DUMMY

#define MARLIN_PORT_HEAT0            MARLIN_PORT_DUMMY
#define MARLIN_PIN_NR_HEAT0          MARLIN_PIN_NR_DUMMY
#define MARLIN_PORT_TEMP_0           MARLIN_PORT_DUMMY
#define MARLIN_PIN_NR_TEMP_0         MARLIN_PIN_NR_DUMMY
#define MARLIN_PORT_TEMP_HEATBREAK   MARLIN_PORT_DUMMY
#define MARLIN_PIN_NR_TEMP_HEATBREAK MARLIN_PIN_NR_DUMMY

namespace buddy::hw {
inline Pin::State zMinReadFn();
inline Pin::State xyProbeReadFn();
} // namespace buddy::hw

// clang-format off
#define PIN_TABLE(MACRO_FUNCTION) \
    MACRO_FUNCTION(buddy::hw::OutputPin, displayCs, buddy::hw::IoPort::D COMMA buddy::hw::IoPin::p11, Pin::State::high COMMA OMode::pushPull COMMA OSpeed::high, buddy::hw::noHandler) \
    MACRO_FUNCTION(buddy::hw::OutputPin, displayRs, buddy::hw::IoPort::D COMMA buddy::hw::IoPin::p15, Pin::State::high COMMA OMode::pushPull COMMA OSpeed::high, buddy::hw::noHandler) \
    MACRO_FUNCTION(buddy::hw::OutputPin, displayRst, buddy::hw::IoPort::G COMMA buddy::hw::IoPin::p4, Pin::State::high COMMA OMode::pushPull COMMA OSpeed::high, buddy::hw::noHandler) \
    MACRO_FUNCTION(buddy::hw::InputPin, jogWheelEN1, buddy::hw::IoPort::D COMMA buddy::hw::IoPin::p13, IMode::input COMMA Pull::up, buddy::hw::noHandler) \
    MACRO_FUNCTION(buddy::hw::InputPin, jogWheelEN2, buddy::hw::IoPort::D COMMA buddy::hw::IoPin::p12, IMode::input COMMA Pull::up, buddy::hw::noHandler) \
    MACRO_FUNCTION(buddy::hw::InputPin, jogWheelENC, buddy::hw::IoPort::G COMMA buddy::hw::IoPin::p3, IMode::input COMMA Pull::up, buddy::hw::noHandler) \
    MACRO_FUNCTION(buddy::hw::OutputPin, xCs, BUDDY_PIN(CS_X), Pin::State::high COMMA OMode::pushPull COMMA OSpeed::high, buddy::hw::noHandler) \
    MACRO_FUNCTION(buddy::hw::OutputPin, yCs, BUDDY_PIN(CS_Y), Pin::State::high COMMA OMode::pushPull COMMA OSpeed::high, buddy::hw::noHandler) \
    MACRO_FUNCTION(buddy::hw::OutputPin, zCs, BUDDY_PIN(CS_Z), Pin::State::high COMMA OMode::pushPull COMMA OSpeed::high, buddy::hw::noHandler) \
    MACRO_FUNCTION(buddy::hw::OutputPin, RS485FlowControlPuppies, buddy::hw::IoPort::G COMMA buddy::hw::IoPin::p1, Pin::State::high COMMA OMode::pushPull COMMA OSpeed::very_high, buddy::hw::noHandler) \
    MACRO_FUNCTION(buddy::hw::OutputPin, RS485FlowControlReserved, buddy::hw::IoPort::G COMMA buddy::hw::IoPin::p2, Pin::State::high COMMA OMode::pushPull COMMA OSpeed::low, buddy::hw::noHandler) \
    MACRO_FUNCTION(buddy::hw::OutputPin, hsUSBEnable, buddy::hw::IoPort::F COMMA buddy::hw::IoPin::p13, Pin::State::high COMMA OMode::pushPull COMMA OSpeed::low, buddy::hw::noHandler) \
    MACRO_FUNCTION(buddy::hw::OutputPin, fsUSBPwrEnable, buddy::hw::IoPort::G COMMA buddy::hw::IoPin::p7, Pin::State::high COMMA OMode::pushPull COMMA OSpeed::low, buddy::hw::noHandler) \
    MACRO_FUNCTION(buddy::hw::InputPin, hsUSBOvercurrent, buddy::hw::IoPort::G COMMA buddy::hw::IoPin::p8, IMode::input COMMA Pull::none, buddy::hw::noHandler) \
    MACRO_FUNCTION(buddy::hw::InputPin, fsUSBOvercurrent, buddy::hw::IoPort::F COMMA buddy::hw::IoPin::p14, IMode::input COMMA Pull::none, buddy::hw::noHandler) \
    MACRO_FUNCTION(buddy::hw::InputPin, fsUSBCInt, buddy::hw::IoPort::F COMMA buddy::hw::IoPin::p3, IMode::input COMMA Pull::none, buddy::hw::noHandler) \
    MACRO_FUNCTION(buddy::hw::InterruptPin, acFault, buddy::hw::IoPort::G COMMA buddy::hw::IoPin::p0, IMode::IT_falling COMMA Pull::none COMMA ISR_PRIORITY_POWER_PANIC COMMA 0, power_panic::ac_fault_isr) \
    MACRO_FUNCTION(buddy::hw::OutputPin, extFlashCs, buddy::hw::IoPort::F COMMA buddy::hw::IoPin::p2, Pin::State::high COMMA OMode::pushPull COMMA OSpeed::high, buddy::hw::noHandler) \
    MACRO_FUNCTION(buddy::hw::OutputPin, AD1setA, buddy::hw::IoPort::F COMMA buddy::hw::IoPin::p15, Pin::State::low COMMA OMode::pushPull COMMA OSpeed::high, buddy::hw::noHandler) \
    MACRO_FUNCTION(buddy::hw::OutputPin, AD1setB, buddy::hw::IoPort::G COMMA buddy::hw::IoPin::p11, Pin::State::low COMMA OMode::pushPull COMMA OSpeed::high, buddy::hw::noHandler) \
    MACRO_FUNCTION(buddy::hw::InterruptPin, xDiag, BUDDY_PIN(X_DIAG), IMode::IT_rising_falling COMMA Pull::none COMMA ISR_PRIORITY_ENDSTOP COMMA 0, endstop_ISR) \
    MACRO_FUNCTION(buddy::hw::InterruptPin, yDiag, BUDDY_PIN(Y_DIAG), IMode::IT_rising_falling COMMA Pull::none COMMA ISR_PRIORITY_ENDSTOP COMMA 0, endstop_ISR) \
    MACRO_FUNCTION(buddy::hw::InterruptPin, zDiag, BUDDY_PIN(Z_DIAG), IMode::IT_rising_falling COMMA Pull::none COMMA ISR_PRIORITY_ENDSTOP COMMA 0, endstop_ISR) \
    MACRO_FUNCTION(buddy::hw::OutputPin, xyEnable, BUDDY_PIN(X_ENA), Pin::State::high COMMA OMode::pushPull COMMA OSpeed::low, buddy::hw::noHandler) \
    MACRO_FUNCTION(buddy::hw::OutputPin, zEnable, BUDDY_PIN(Z_ENA), Pin::State::high COMMA OMode::pushPull COMMA OSpeed::low, buddy::hw::noHandler) \
    MACRO_FUNCTION(buddy::hw::OutputPin, zStep, BUDDY_PIN(Z_STEP), Pin::State::low COMMA OMode::pushPull COMMA OSpeed::very_high, buddy::hw::noHandler) \
    MACRO_FUNCTION(buddy::hw::OutputPin, e0Step, BUDDY_PIN(E0_STEP), Pin::State::low COMMA OMode::pushPull COMMA OSpeed::very_high, buddy::hw::noHandler) \
    MACRO_FUNCTION(buddy::hw::OutputPin, xDir, BUDDY_PIN(X_DIR), Pin::State::low COMMA OMode::pushPull COMMA OSpeed::very_high, buddy::hw::noHandler) \
    MACRO_FUNCTION(buddy::hw::OutputPin, yDir, BUDDY_PIN(Y_DIR), Pin::State::low COMMA OMode::pushPull COMMA OSpeed::very_high, buddy::hw::noHandler) \
    MACRO_FUNCTION(buddy::hw::OutputPin, zDir, BUDDY_PIN(Z_DIR), Pin::State::low COMMA OMode::pushPull COMMA OSpeed::very_high, buddy::hw::noHandler) \
    MACRO_FUNCTION(buddy::hw::OutputPin, e0Dir, BUDDY_PIN(E0_DIR), Pin::State::low COMMA OMode::pushPull COMMA OSpeed::very_high, buddy::hw::noHandler) \
    MACRO_FUNCTION(buddy::hw::InputOutputPin, touch_sig, buddy::hw::IoPort::C COMMA buddy::hw::IoPin::p8, IMode::input COMMA Pull::none, buddy::hw::noHandler) \
    MACRO_FUNCTION(buddy::hw::OutputPin, splitter5vEnable, buddy::hw::IoPort::E COMMA buddy::hw::IoPin::p14, Pin::State::high COMMA OMode::pushPull COMMA OSpeed::low, buddy::hw::noHandler) \
    MACRO_FUNCTION(buddy::hw::OutputPin, AD2setA, buddy::hw::IoPort::F COMMA buddy::hw::IoPin::p12, Pin::State::low COMMA OMode::pushPull COMMA OSpeed::high, buddy::hw::noHandler) \
    MACRO_FUNCTION(buddy::hw::OutputPin, AD2setB, buddy::hw::IoPort::G COMMA buddy::hw::IoPin::p6, Pin::State::low COMMA OMode::pushPull COMMA OSpeed::high, buddy::hw::noHandler) \
    MACRO_FUNCTION(buddy::hw::OutputPin, espPower, buddy::hw::IoPort::F COMMA buddy::hw::IoPin::p4, Pin::State::low COMMA OMode::pushPull COMMA OSpeed::low, buddy::hw::noHandler) \
    MACRO_FUNCTION(buddy::hw::OutputPin, GPIOReset, buddy::hw::IoPort::E COMMA buddy::hw::IoPin::p7, Pin::State::high COMMA OMode::openDrain COMMA OSpeed::low, buddy::hw::noHandler) \
    MACRO_FUNCTION(buddy::hw::InputPin, fan1_tach0, buddy::hw::IoPort::G COMMA buddy::hw::IoPin::p5, IMode::input COMMA Pull::none, buddy::hw::noHandler)

// These pins shall not be configured automatically. The intialization code shall determine what gets initialized based on board revision
#define RUNTIME_PIN_TABLE(MACRO_FUNCTION) \
    MACRO_FUNCTION(buddy::hw::OutputPin, buzzer_pin_d5, buddy::hw::IoPort::D COMMA buddy::hw::IoPin::p5, Pin::State::low COMMA OMode::pushPull COMMA OSpeed::low, buddy::hw::noHandler) \
    MACRO_FUNCTION(buddy::hw::OutputPin, buzzer_pin_a0, buddy::hw::IoPort::A COMMA buddy::hw::IoPin::p0, Pin::State::low COMMA OMode::pushPull COMMA OSpeed::low, buddy::hw::noHandler) \
    MACRO_FUNCTION(buddy::hw::OutputPin, xStep_pin_a0, buddy::hw::IoPort::A COMMA buddy::hw::IoPin::p0, Pin::State::low COMMA OMode::pushPull COMMA OSpeed::very_high, buddy::hw::noHandler) \
    MACRO_FUNCTION(buddy::hw::OutputPin, xStep_pin_d7, buddy::hw::IoPort::D COMMA buddy::hw::IoPin::p7, Pin::State::low COMMA OMode::pushPull COMMA OSpeed::very_high, buddy::hw::noHandler) \
    MACRO_FUNCTION(buddy::hw::OutputPin, yStep_pin_a3, buddy::hw::IoPort::A COMMA buddy::hw::IoPin::p3, Pin::State::low COMMA OMode::pushPull COMMA OSpeed::very_high, buddy::hw::noHandler) \
    MACRO_FUNCTION(buddy::hw::OutputPin, yStep_pin_d5, buddy::hw::IoPort::D COMMA buddy::hw::IoPin::p5, Pin::State::low COMMA OMode::pushPull COMMA OSpeed::very_high, buddy::hw::noHandler) \
    MACRO_FUNCTION(buddy::hw::OutputPin, sideLed_LcdSelector_pin_e9, buddy::hw::IoPort::E COMMA buddy::hw::IoPin::p9, Pin::State::low COMMA OMode::pushPull COMMA OSpeed::low, buddy::hw::noHandler)

// clang-format on

namespace buddy::hw {

extern PCA9557 io_expander1;

// Following pins are connected differently depending on HW revision -> we'll determine where they are during runtime in hwio_configure_board_revision_changed_pins
extern const OutputPin *Buzzer;
extern const OutputPin *XStep;
extern const OutputPin *YStep;
extern const OutputPin *SideLed_LcdSelector;

} // namespace buddy::hw

// clang-format off
#define EXTENDER_PIN_TABLE(MACRO_FUNCTION) \
    MACRO_FUNCTION(buddy::hw::PCA9557OutputPin, dwarf6Reset, buddy::hw::IoPin::p0, Pin::State::low COMMA io_expander1, buddy::hw::noHandler) \
    MACRO_FUNCTION(buddy::hw::PCA9557OutputPin, dwarf1Reset, buddy::hw::IoPin::p1, Pin::State::low COMMA io_expander1, buddy::hw::noHandler) \
    MACRO_FUNCTION(buddy::hw::PCA9557OutputPin, dwarf2Reset, buddy::hw::IoPin::p2, Pin::State::low COMMA io_expander1, buddy::hw::noHandler) \
    MACRO_FUNCTION(buddy::hw::PCA9557OutputPin, dwarf3Reset, buddy::hw::IoPin::p3, Pin::State::low COMMA io_expander1, buddy::hw::noHandler) \
    MACRO_FUNCTION(buddy::hw::PCA9557OutputPin, dwarf4Reset, buddy::hw::IoPin::p4, Pin::State::low COMMA io_expander1, buddy::hw::noHandler) \
    MACRO_FUNCTION(buddy::hw::PCA9557OutputPin, dwarf5Reset, buddy::hw::IoPin::p5, Pin::State::low COMMA io_expander1, buddy::hw::noHandler) \
    MACRO_FUNCTION(buddy::hw::PCA9557OutputPin, fanPowerSwitch, buddy::hw::IoPin::p6, Pin::State::low COMMA io_expander1, buddy::hw::noHandler) \
    MACRO_FUNCTION(buddy::hw::PCA9557OutputPin, modularBedReset, buddy::hw::IoPin::p7, Pin::State::low COMMA io_expander1, buddy::hw::noHandler)

#define VIRTUAL_PIN_TABLE(MACRO_FUNCTION) \
    MACRO_FUNCTION(buddy::hw::VirtualInterruptPin, buddy::hw::zMinReadFn, endstop_ISR, zMin, BUDDY_PIN(Z_MIN), IMode::IT_rising_falling) \
    MACRO_FUNCTION(buddy::hw::VirtualInterruptPin, buddy::hw::xyProbeReadFn, endstop_ISR, xyProbe, BUDDY_PIN(XY_PROBE), IMode::IT_rising_falling)
// clang-format on

#define HAS_ZMIN_READ_FN    1
#define HAS_XYPROBE_READ_FN 1

/**
 * @brief Return SPI handle that controls side LEDS
 * @note It is either LCD spi or separate SPI for side leds, depending on hwrevision
 */
SPI_HandleTypeDef *hw_get_spi_side_strip();
