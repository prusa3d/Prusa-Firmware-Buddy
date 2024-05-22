#pragma once
// this is unused pin, we map everything that is uniused but referenced to avoid colisions
#define MARLIN_PORT_DUMMY   MARLIN_PORT_B
#define MARLIN_PIN_NR_DUMMY MARLIN_PIN_NR_5

#define MARLIN_PORT_E0_DIR    MARLIN_PORT_D
#define MARLIN_PIN_NR_E0_DIR  MARLIN_PIN_NR_0
#define MARLIN_PORT_E0_STEP   MARLIN_PORT_D
#define MARLIN_PIN_NR_E0_STEP MARLIN_PIN_NR_1
#define MARLIN_PORT_E0_ENA    MARLIN_PORT_D
#define MARLIN_PIN_NR_E0_ENA  MARLIN_PIN_NR_2

#define MARLIN_PORT_X_STEP   MARLIN_PORT_DUMMY
#define MARLIN_PIN_NR_X_STEP MARLIN_PIN_NR_DUMMY
#define MARLIN_PORT_X_DIR    MARLIN_PORT_DUMMY
#define MARLIN_PIN_NR_X_DIR  MARLIN_PIN_NR_DUMMY
#define MARLIN_PORT_X_ENA    MARLIN_PORT_DUMMY
#define MARLIN_PIN_NR_X_ENA  MARLIN_PIN_NR_DUMMY

#define MARLIN_PORT_Y_STEP   MARLIN_PORT_DUMMY
#define MARLIN_PIN_NR_Y_STEP MARLIN_PIN_NR_DUMMY
#define MARLIN_PORT_Y_DIR    MARLIN_PORT_DUMMY
#define MARLIN_PIN_NR_Y_DIR  MARLIN_PIN_NR_DUMMY
#define MARLIN_PORT_Y_ENA    MARLIN_PORT_DUMMY
#define MARLIN_PIN_NR_Y_ENA  MARLIN_PIN_NR_DUMMY

#define MARLIN_PORT_Z_STEP   MARLIN_PORT_DUMMY
#define MARLIN_PIN_NR_Z_STEP MARLIN_PIN_NR_DUMMY
#define MARLIN_PORT_Z_DIR    MARLIN_PORT_DUMMY
#define MARLIN_PIN_NR_Z_DIR  MARLIN_PIN_NR_DUMMY
#define MARLIN_PORT_Z_ENA    MARLIN_PORT_DUMMY
#define MARLIN_PIN_NR_Z_ENA  MARLIN_PIN_NR_DUMMY

#define MARLIN_PORT_X_DIAG   MARLIN_PORT_DUMMY
#define MARLIN_PIN_NR_X_DIAG MARLIN_PIN_NR_DUMMY
#define MARLIN_PORT_Y_DIAG   MARLIN_PORT_DUMMY
#define MARLIN_PIN_NR_Y_DIAG MARLIN_PIN_NR_DUMMY
#define MARLIN_PORT_Z_DIAG   MARLIN_PORT_DUMMY
#define MARLIN_PIN_NR_Z_DIAG MARLIN_PIN_NR_DUMMY
#define MARLIN_PORT_Z_MIN    MARLIN_PORT_DUMMY
#define MARLIN_PIN_NR_Z_MIN  MARLIN_PIN_NR_DUMMY

#define MARLIN_PORT_CS_X   MARLIN_PORT_DUMMY
#define MARLIN_PIN_NR_CS_X MARLIN_PIN_NR_DUMMY
#define MARLIN_PORT_CS_Y   MARLIN_PORT_DUMMY
#define MARLIN_PIN_NR_CS_Y MARLIN_PIN_NR_DUMMY
#define MARLIN_PORT_CS_Z   MARLIN_PORT_DUMMY
#define MARLIN_PIN_NR_CS_Z MARLIN_PIN_NR_DUMMY
#define MARLIN_PORT_CS_E   MARLIN_PORT_B
#define MARLIN_PIN_NR_CS_E MARLIN_PIN_NR_4

#define MARLIN_PORT_TEMP_0   MARLIN_PORT_A
#define MARLIN_PIN_NR_TEMP_0 MARLIN_PIN_NR_5

#define MARLIN_PORT_TEMP_HEATBREAK   MARLIN_PORT_B
#define MARLIN_PIN_NR_TEMP_HEATBREAK MARLIN_PIN_NR_2

#define MARLIN_PORT_TEMP_BOARD   MARLIN_PORT_A
#define MARLIN_PIN_NR_TEMP_BOARD MARLIN_PIN_NR_7

// FAN is print fan, on schematic marked as "FAN-1-PWM"
#define MARLIN_PORT_FAN   MARLIN_PORT_C
#define MARLIN_PIN_NR_FAN MARLIN_PIN_NR_7

// FAN1 is heatbreak fan, on schematic marked as "FAN-0-PWM"
#define MARLIN_PORT_FAN1   MARLIN_PORT_C
#define MARLIN_PIN_NR_FAN1 MARLIN_PIN_NR_6

// FAN is print fan, on schematic marked as "FAN-1-TACH"
#define MARLIN_PORT_FAN_TACH0   MARLIN_PORT_C
#define MARLIN_PIN_NR_FAN_TACH0 MARLIN_PIN_NR_9

// FAN1 is heatbreak fan, on schematic marked as "FAN-0-TACH"
#define MARLIN_PORT_FAN1_TACH0   MARLIN_PORT_C
#define MARLIN_PIN_NR_FAN1_TACH0 MARLIN_PIN_NR_8

#define MARLIN_PORT_HEAT0   MARLIN_PORT_A
#define MARLIN_PIN_NR_HEAT0 MARLIN_PIN_NR_6

#define MARLIN_PORT_LOCALREMOTE   MARLIN_PORT_D
#define MARLIN_PIN_NR_LOCALREMOTE MARLIN_PIN_NR_4

#define MARLIN_PORT_NEOPIXEL   MARLIN_PORT_B
#define MARLIN_PIN_NR_NEOPIXEL MARLIN_PIN_NR_6

#define MARLIN_PORT_BUTTON1   MARLIN_PORT_A
#define MARLIN_PIN_NR_BUTTON1 MARLIN_PIN_NR_15

#define MARLIN_PORT_BUTTON2   MARLIN_PORT_C
#define MARLIN_PIN_NR_BUTTON2 MARLIN_PIN_NR_10

#define MARLIN_PORT_PANIC   MARLIN_PORT_A
#define MARLIN_PIN_NR_PANIC MARLIN_PIN_NR_11

#define MARLIN_PORT_HWID0   MARLIN_PORT_C
#define MARLIN_PIN_NR_HWID0 MARLIN_PIN_NR_0

#define MARLIN_PORT_HWID1   MARLIN_PORT_C
#define MARLIN_PIN_NR_HWID1 MARLIN_PIN_NR_1

#define MARLIN_PORT_HWID2   MARLIN_PORT_A
#define MARLIN_PIN_NR_HWID2 MARLIN_PIN_NR_2

#define MARLIN_PORT_HWID3   MARLIN_PORT_C
#define MARLIN_PIN_NR_HWID3 MARLIN_PIN_NR_3

#define MARLIN_PORT_LED   MARLIN_PORT_C
#define MARLIN_PIN_NR_LED MARLIN_PIN_NR_11

#define MARLIN_PORT_ACC_CS   MARLIN_PORT_B
#define MARLIN_PIN_NR_ACC_CS MARLIN_PIN_NR_11

namespace buddy::hw {
Pin::State zMinReadFn();

inline Pin::State zMinReadFn() {
    return Pin::State::high;
}
} // namespace buddy::hw

// clang-format off
#define PIN_TABLE(MACRO_FUNCTION) \
    MACRO_FUNCTION(buddy::hw::OutputPin, dummy, BUDDY_PIN(DUMMY), Pin::State::high COMMA OMode::pushPull COMMA OSpeed::low, buddy::hw::noHandler) \
    MACRO_FUNCTION(buddy::hw::OutputPin, e0Enable, BUDDY_PIN(E0_ENA), Pin::State::high COMMA OMode::pushPull COMMA OSpeed::low, buddy::hw::noHandler) \
    MACRO_FUNCTION(buddy::hw::OutputPin, e0Step, BUDDY_PIN(E0_STEP), Pin::State::low COMMA OMode::pushPull COMMA OSpeed::low, buddy::hw::noHandler) \
    MACRO_FUNCTION(buddy::hw::OutputPin, e0Dir, BUDDY_PIN(E0_DIR), Pin::State::low COMMA OMode::pushPull COMMA OSpeed::low, buddy::hw::noHandler) \
    MACRO_FUNCTION(buddy::hw::OutputPin, heat0, BUDDY_PIN(HEAT0), Pin::State::low COMMA OMode::pushPull COMMA OSpeed::low, buddy::hw::noHandler) \
    MACRO_FUNCTION(buddy::hw::OutputPin, fanPrintPwm, BUDDY_PIN(FAN), Pin::State::low COMMA OMode::pushPull COMMA OSpeed::low, buddy::hw::noHandler) \
    MACRO_FUNCTION(buddy::hw::OutputPin, localRemote, BUDDY_PIN(LOCALREMOTE), Pin::State::low COMMA OMode::pushPull COMMA OSpeed::low, buddy::hw::noHandler) \
    MACRO_FUNCTION(buddy::hw::OutputPin, neopixel, BUDDY_PIN(NEOPIXEL), Pin::State::low COMMA OMode::pushPull COMMA OSpeed::very_high, buddy::hw::noHandler) \
    MACRO_FUNCTION(buddy::hw::InterruptPin, hx717Dout, buddy::hw::IoPort::A COMMA buddy::hw::IoPin::p3, IMode::IT_falling COMMA Pull::up COMMA ISR_PRIORITY_HX717 COMMA 0 COMMA false, dwarf::loadcell::loadcell_irq) \
    MACRO_FUNCTION(buddy::hw::OutputPin, hx717Sck, buddy::hw::IoPort::A COMMA buddy::hw::IoPin::p0, Pin::State::low COMMA OMode::pushPull COMMA OSpeed::very_high, buddy::hw::noHandler) \
    MACRO_FUNCTION(buddy::hw::OutputPin, fanHeatBreakPwm, BUDDY_PIN(FAN1), Pin::State::low COMMA OMode::pushPull COMMA OSpeed::low, buddy::hw::noHandler) \
    MACRO_FUNCTION(buddy::hw::InputPin, button1, BUDDY_PIN(BUTTON1), IMode::input COMMA Pull::up, buddy::hw::noHandler) \
    MACRO_FUNCTION(buddy::hw::InputPin, button2, BUDDY_PIN(BUTTON2), IMode::input COMMA Pull::up, buddy::hw::noHandler) \
    MACRO_FUNCTION(buddy::hw::InputPin, acFault, BUDDY_PIN(PANIC), IMode::input COMMA Pull::none, buddy::hw::noHandler) \
    MACRO_FUNCTION(buddy::hw::InputPin, hwid0, BUDDY_PIN(HWID0), IMode::input COMMA Pull::none, buddy::hw::noHandler) \
    MACRO_FUNCTION(buddy::hw::InputPin, hwid1, BUDDY_PIN(HWID1), IMode::input COMMA Pull::none, buddy::hw::noHandler) \
    MACRO_FUNCTION(buddy::hw::InputPin, hwid2, BUDDY_PIN(HWID2), IMode::input COMMA Pull::none, buddy::hw::noHandler) \
    MACRO_FUNCTION(buddy::hw::InputPin, hwid3, BUDDY_PIN(HWID3), IMode::input COMMA Pull::none, buddy::hw::noHandler) \
    MACRO_FUNCTION(buddy::hw::OutputPin, ledPWM, BUDDY_PIN(LED), Pin::State::low COMMA OMode::pushPull COMMA OSpeed::low, buddy::hw::noHandler) \
    MACRO_FUNCTION(buddy::hw::InputPin, fanPrintTach, BUDDY_PIN(FAN_TACH0), IMode::input COMMA Pull::none, buddy::hw::noHandler) \
    MACRO_FUNCTION(buddy::hw::InputPin, fanHeatBreakTach, BUDDY_PIN(FAN1_TACH0), IMode::input COMMA Pull::none, buddy::hw::noHandler) \
    MACRO_FUNCTION(buddy::hw::OutputPin, acellCs, BUDDY_PIN(ACC_CS), Pin::State::high COMMA OMode::pushPull COMMA OSpeed::high, buddy::hw::noHandler) \
    MACRO_FUNCTION(buddy::hw::InterruptPin, lis2dh12_data, buddy::hw::IoPort::B COMMA buddy::hw::IoPin::p1, IMode::IT_rising COMMA Pull::down COMMA ISR_PRIORITY_LIS2DH12 COMMA 0, dwarf::accelerometer::irq)

#define VIRTUAL_PIN_TABLE(MACRO_FUNCTION) \
    MACRO_FUNCTION(buddy::hw::VirtualInterruptPin, buddy::hw::zMinReadFn, endstop_ISR, zMin, BUDDY_PIN(Z_MIN), IMode::IT_rising_falling)
// clang-format on

#define HAS_ZMIN_READ_FN 0
