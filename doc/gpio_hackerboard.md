# GPIO Hackerboard - Technical Documentation

> Line numbers may drift with new versions — primarily navigate by function and symbol names.

## 1. Overview

The GPIO Hackerboard is an expansion board built around the **TCA6408A I2C GPIO expander**, providing 8 additional GPIO pins. The firmware never calls it "Hackerboard" — everything is referred to as the *I2C IO expander* behind the `HAS_I2C_EXPANDER()` feature macro.

### Hardware specification
- **Chip:** TCA6408A (Texas Instruments)
- **I2C address:** `0x20` (7-bit), `0x40`/`0x41` (8-bit write/read)
- **I2C channel:** I2C2 (xBuddy boards only, see chapter 10)
- **Bus speed:** 400 kHz (`i2c2_speed`, `src/device/stm32f4/peripherals.cpp:542`)
- **Pin count:** 8x GPIO (P0-P7)
- **Timeout:** 5 ms per I2C operation
- **Chip INT pin:** unused by the firmware — inputs are read by polling

### Board pins and electrical limits (from the official board documentation)

| Pins | Mode | Limits |
|------|------|--------|
| 0-3 | **output only** (open-drain, power) | switch up to **24 V / 500 mA** to GND |
| 4-7 | input or output (IO with an internal pull-up to 3.3 V) | output: 0 V / max. 3.3 V, **10 mA**; input: 0–3.3 V (max. short-circuit current 0.5 mA, < 4 kΩ) |

According to the official description the board outputs are **open-drain**: an active pin is switched to ground (0 V) by a transistor, an inactive pin "floats" — on pins 4-7 the internal pull-up holds it at ~3.3 V, on pins 0-3 the level is defined by the external load/pull-up (up to 24 V).

Pins 0-3 are also routed to **2.5 mm jack connectors** — pin 0 is conventionally used as a camera trigger for timelapse. Tip: trigger the camera on every layer with the volatile `M266 P0 B1`/`B0` (see chapter 5) instead of `M264` — it saves thousands of EEPROM writes per print.

Consequences:
- **Pins 0-3 cannot be used as inputs** — the limitation is in hardware (the power transistor); the firmware does not prevent configuring `M262 P0-3 B1`, but the connector will not read meaningful values.
- **Connector polarity**: on pins 0-3, writing `B1` means "switch the load to GND" — the logic level at the connector is therefore **inverted** relative to the Output register value. On pins 4-7, writing `B1` corresponds to ~3.3 V and `B0` to 0 V. Verify the polarity of a particular pin with a multimeter before wiring external electronics.
- **Never apply more than 3.3 V to the pins** (inputs 4-7); 5V logic (e.g. Arduino UNO) may only pull a pin to GND (open-drain emulation: `OUTPUT`+`LOW` ↔ `INPUT`), never actively drive HIGH.

---

## 2. Architecture and key files

```
┌─────────────────────────────────────────────────────────────────┐
│                       Application layer                         │
├─────────────────────────────────────────────────────────────────┤
│  G-codes M262-M269             │  Button Macro system           │
│  src/marlin_stubs/M262-M269.cpp│  src/common/marlin_server.cpp  │
│                                │  src/common/gcode/inject_queue │
├─────────────────────────────────────────────────────────────────┤
│                         Driver layer                            │
├─────────────────────────────────────────────────────────────────┤
│  TCA6408A class                                                 │
│  src/hw/TCA6408A.hpp                                            │
│  src/hw/TCA6408A.cpp                                            │
├─────────────────────────────────────────────────────────────────┤
│                        I2C abstraction                          │
├─────────────────────────────────────────────────────────────────┤
│  src/common/i2c.hpp / i2c.cpp                                   │
├─────────────────────────────────────────────────────────────────┤
│                       Persistent Storage                        │
├─────────────────────────────────────────────────────────────────┤
│  src/persistent_stores/store_instances/config_store/            │
│  store_definition.hpp                                           │
├─────────────────────────────────────────────────────────────────┤
│                           Hardware                              │
├─────────────────────────────────────────────────────────────────┤
│  src/device/stm32f4/peripherals.cpp   (io_expander2 instance)   │
│  src/buddy/main.cpp                   (initialization)          │
└─────────────────────────────────────────────────────────────────┘
```

### Files and their purpose

| File | Purpose |
|------|---------|
| `src/hw/TCA6408A.hpp` | TCA6408A class declaration - the main driver |
| `src/hw/TCA6408A.cpp` | Implementation of the I2C communication with the chip |
| `src/marlin_stubs/M262-M269.cpp` | G-codes for controlling the expander (incl. volatile write M266 and wait-for-pin M269) |
| `src/marlin_stubs/io_expander_utils.hpp` | Shared helpers (volatile write, pin direction check) |
| `src/marlin_stubs/gcode.cpp:115-134` | G-code registration (behind `#if HAS_I2C_EXPANDER()`) |
| `src/common/marlin_server.cpp:749-792` | Button macro loop - automatic input reading (`io_expander_read_loop`), called from `cycle()` at line 869 |
| `src/common/gcode/inject_queue.cpp:50-58` | Building the `btn_<pin>` macro file name after a button press |
| `src/common/gcode/gcode_loader.cpp:106` | Loading the macro file from `/usb/macros/<name>.gcode` |
| `src/device/stm32f4/peripherals.cpp:77` | Creation of the global `io_expander2` instance |
| `src/buddy/main.cpp:484` | Initialization at firmware startup |
| `src/persistent_stores/.../store_definition.hpp:632-634` | Persistent storage definitions |
| `include/device/stm32f4/device/peripherals.h` | Per-board mapping of `i2c_io_expander2` to a bus |
| `ProjectOptions.cmake:542` | `HAS_I2C_EXPANDER` feature flag |

---

## 3. TCA6408A registers

| Register | Enum value | Register address | Access | Description |
|----------|------------|------------------|--------|-------------|
| Input | `Register::Input` | `0x00` | Read-only | Current pin states |
| Output | `Register::Output` | `0x01` | Read/Write | Output values |
| Polarity | `Register::Polarity` | `0x02` | Read/Write | Input inversion (1=inverted) |
| Config | `Register::Config` | `0x03` | Read/Write | Pin direction (1=input, 0=output) |

### Register notes
- **Config register:** EEPROM default is 0 = all pins as outputs
- **Polarity register:** inverts the value read from the Input register
- The Output, Config and Polarity registers are stored in EEPROM (config_store) and restored to the chip at boot

---

## 4. API - the TCA6408A class

### Definition (src/hw/TCA6408A.hpp)

```cpp
namespace buddy::hw {

class TCA6408A {
public:
    enum class Register : uint8_t {
        Input = 0,    // Input reading (read-only)
        Output = 1,   // Output states
        Polarity = 2, // Polarity inversion
        Config = 3,   // Pin direction (1=input, 0=output)
    };

    static constexpr uint8_t pin_count = 8;
    static constexpr uint8_t fixed_addr = 0x20;

    // Constructor - takes an I2C handle
    constexpr TCA6408A(I2C_HandleTypeDef &i2c);

    // Initialization - loads the registers from EEPROM and writes them to the chip
    void initialize();

    // Output write to selected pins (Output register)
    bool write(uint8_t value, uint8_t pin_mask = 0xFF);

    // Input read of selected pins
    // Returns std::nullopt on an I2C error
    std::optional<uint8_t> read(uint8_t pin_mask = 0xFF) const;

    // Toggle of a single pin in the Output register
    bool toggle(uint8_t pin_number);

    // Update of any register (except Input)
    // Persists the change to config_store
    bool update_register(Register reg, uint8_t value, uint8_t pin_mask = 0xFF);

    // Direct register read/write (without persistence)
    bool read_reg(Register reg, uint8_t &value) const;
    bool write_reg(Register reg, uint8_t value);

    // Initialization check
    inline bool is_initialized() { return is_initialized_; }

private:
    I2C_HandleTypeDef &i2c;
    const uint8_t read_address;   // 0x41 (0x20 << 1 | 1)
    const uint8_t write_address;  // 0x40 (0x20 << 1)
    static constexpr uint32_t timeout_ms = 5;
    bool is_initialized_ = false;
};

// Global instance
extern TCA6408A io_expander2;

} // namespace buddy::hw
```

### Important method behavior (from the TCA6408A.cpp implementation)

- **`write()` does not change the Config register.** If the target pins are not configured as outputs, it only prints a warning to the serial port (`SERIAL_ERROR_MSG`) and performs the Output register write anyway. The direction configuration must be done separately (`M262` / `update_register(Register::Config, ...)`).
- **`read()` analogously:** if the read pins are not configured as inputs, it prints a warning but still reads the Input register. Bits outside `pin_mask` are returned as 0.
- **`update_register()`:**
  - Rejects the Input register (returns `false`).
  - With a partial mask (`pin_mask != 0xFF`) it loads the rest of the byte from config_store, and if the resulting value does not differ from the stored one, **the I2C write is skipped** (returns `true` without communicating).
  - After a successful write it stores the value to config_store (EEPROM).
- **`toggle()`** reads the current Output register directly from the chip (`read_reg`), XORs the selected bit, writes it back and stores it to config_store.
- **`read_reg()` / `write_reg()`** fail with an error on the serial port when the expander is not initialized (`is_initialized_ == false`). They do not persist anything to EEPROM.
- The header contains a private member `triggered_states` — an unused leftover; button debouncing is implemented in `marlin_server.cpp` (see chapter 6).

### Code usage examples

```cpp
#include "TCA6408A.hpp"

// Configure pin 0 as an output and turn it on
buddy::hw::io_expander2.update_register(
    buddy::hw::TCA6408A::Register::Config, 0x00, 0x01);  // pin 0 = output
buddy::hw::io_expander2.write(0x01, 0x01);               // pin 0 = HIGH

// Read the state of pin 3
if (auto val = buddy::hw::io_expander2.read(0x08)) {  // mask=bit3
    bool pin3_high = (*val & 0x08) != 0;
}

// Toggle pin 5
buddy::hw::io_expander2.toggle(5);

// Configure pins 0-3 as inputs
buddy::hw::io_expander2.update_register(
    buddy::hw::TCA6408A::Register::Config,
    0x0F,  // value: bits 0-3 = 1 (input)
    0x0F   // mask: change only bits 0-3
);
```

---

## 5. Control G-codes

Available only with `HAS_I2C_EXPANDER` (see chapter 10). The registration in `src/marlin_stubs/gcode.cpp` is entirely behind `#if HAS_I2C_EXPANDER()`. Note: the doxygen comments of M262-M265/M267/M268 in `M262-M269.cpp` mention only "MK3.5/S, MK3.9/S, MK4/S" — they are outdated, the flag also applies to Core One (see chapter 10).

The family is complemented by **M266** (volatile write without EEPROM) and **M269** (blocking wait for a pin state) — described below.

On an I2C communication failure all the G-codes print ` Error: Failed i2c communication`. If the boot-time initialization failed (board not connected), the driver prints ` IO Expander not initialized properly`.

### M262 - Pin configuration

Configures a pin as an input or an output (writes the Config register, persisted to EEPROM).

```gcode
M262 P<pin> B<mode>
```
- `P` - pin number (0-7); **only pins 4-7 make sense as inputs on this board** (0-3 are output-only in hardware, see chapter 1)
- `B` - mode: `0` = output, non-zero value (1-255) = input

Examples:
```gcode
M262 P0 B0  ; Pin 0 as an output
M262 P5 B1  ; Pin 5 as an input
```

### M263 - Input read

Reads the state of a pin or of the whole Input register.

```gcode
M263 [P<pin>]
```
- `P` - optional, pin number (0-7)

Examples and serial port output:
```gcode
M263      ; "IO Expander Input Register: <0-255>"
M263 P6   ; "IO Expander Pin HIGH" or "IO Expander Pin LOW"
```

### M264 - Output write

Sets the output value of a pin (persisted to EEPROM). If the pin is not configured as an output, the write still happens, but a warning is printed to the serial port.

```gcode
M264 P<pin> B<value>
```
- `P` - pin number (0-7)
- `B` - value: `0` = LOW, non-zero value = HIGH

Examples:
```gcode
M264 P0 B1  ; Pin 0 HIGH
M264 P7 B0  ; Pin 7 LOW
```

### M265 - Output toggle

Toggles the state of an output pin (persisted to EEPROM). Does not check whether the pin is configured as an output.

```gcode
M265 P<pin>
```

Example:
```gcode
M265 P0  ; Toggle pin 0 (HIGH->LOW or LOW->HIGH)
```

### M266 - Volatile output write (no EEPROM)

Like `M264`/`M267 R1`, but **does not persist the value to EEPROM**. Intended for frequent signalling (a camera trigger on every layer, signals to an external device on every filament change) where `M264`/`M267` would needlessly wear out the config_store journal. After a printer restart the Output register is restored from the last value stored via `M264`/`M267`.

```gcode
M266 P<pin> B<0/1>          ; single pin write
M266 R1 B<byte> [M<mask>]   ; Output register write (optionally only selected bits)
```

- `P` - pin (0-7)
- `B` - value: with `P` non-zero = active bit; with `R` the whole byte (0-255)
- `R` - register variant; only `R1` (Output) is allowed
- `M` - bit mask for the `R` variant: only the masked bits change, the others keep their state (read-modify-write; the final I2C write is a single one → the masked bits change atomically). Default 255.

Examples:
```gcode
M266 P0 B1        ; activate pin 0 (timelapse trigger), no EEPROM write
M266 R1 B6 M14    ; set pins 1-3 at once, leave pins 0 and 4-7 untouched
```

### M267 - Register write

Direct register write (overwrites the whole byte, persisted to EEPROM). An attempt to write the Input register (`R0`) is rejected with ` Aborted. Input register is read only.`

```gcode
M267 R<register> B<value>
```
- `R` - register: `1` = Output, `2` = Polarity, `3` = Config
- `B` - value (0-255)

Examples:
```gcode
M267 R3 B255  ; Config = 0xFF (all pins as inputs)
M267 R1 B0    ; Output = 0x00 (all outputs LOW)
```

### M268 - Register read

Reads the register value and prints it as a number to the serial port.

```gcode
M268 R<register>
```
- `R` - register: `0` = Input, `1` = Output, `2` = Polarity, `3` = Config

Example:
```gcode
M268 R0  ; Read the Input register
M268 R3  ; Read the Config register
```

### M269 - Wait for pin state

Blocks G-code processing until the input pin reaches the requested state — a replacement for Marlin's `M226`, which is not available in the Buddy firmware. The print head stays where it is, temperatures are kept, the print is not paused (from the firmware's point of view it is one long G-code, like an `M109` wait). Like `M263`, it **only reads** the pin — it does not change the pin direction; configure it beforehand via `M262`.

```gcode
M269 P<pin> [S<0/1>] [T<seconds>]
```

- `P` - pin (0-7); only pins 4-7 are input-capable on this board (see chapter 1)
- `S` - state to wait for: `1` = HIGH (default), `0` = LOW
- `T` - timeout in seconds; without it the wait is unlimited

Behavior:
- The pin is sampled every 100 ms (an expander read blocks for ~5 ms on I2C).
- Timeout → a message on the serial port + print pause (an `M601` equivalent) and the G-code finishes.
- Aborting the print ends the wait correctly; I2C errors are not fatal (covered by the timeout).

Example:
```gcode
M269 P6 S0 T300  ; wait up to 5 minutes for an external device to pull pin 6 to GND
```

---

## 6. Button Macro system

An automatic system detecting presses of buttons connected to the GPIO expander. A button press runs a G-code macro from a USB flash drive.

### Implementation (src/common/marlin_server.cpp:763, `io_expander_read_loop`)

The function is called from the marlin server main loop (`cycle()`). The logic:

1. If the expander is not initialized (`is_initialized()`), it does nothing.
2. If the Config register in config_store is **zero** (no pin configured as an input), it does nothing — the polling does not run at all.
3. Otherwise every **500 ms** (`io_expander_read_loop_delay_ms`) it reads the Input register (only the pins per the Config register).
4. Debouncing/edge detection in `io_expander_button_trigger_check()` — it only detects the released → pressed transition; after a press the pin must be released before it can trigger again.
5. For every newly pressed pin it injects `GCodeMacroButton(pin)`; on an injection failure it logs `"Injecting Macro Button failed, pin: N"`.

```cpp
// Debouncing mechanism (marlin_server.cpp:752)
static uint8_t io_expander_button_trigger_check(uint8_t pin_states, uint8_t pin_mask) {
    static uint8_t prev_pressed_buttons = 0;

    // Inverted - pins are LOW when pressed
    const auto pressed_buttons = (~pin_states) & pin_mask;

    // Edge detection (newly pressed)
    const auto triggered_buttons = pressed_buttons & ~prev_pressed_buttons;

    prev_pressed_buttons = pressed_buttons;
    return triggered_buttons;
}
```

### What happens after a press - macro files on USB

`GCodeMacroButton(pin)` goes through the inject queue (`src/common/gcode/inject_queue.cpp`), where the file name `btn_<pin>` is built, and `GCodeLoader` (`src/common/gcode/gcode_loader.cpp`) asynchronously loads and executes the file:

```
/usb/macros/btn_0.gcode   ; pin 0
/usb/macros/btn_1.gcode   ; pin 1
...
/usb/macros/btn_7.gcode   ; pin 7
```

The file must exist on the connected USB flash drive in the `macros` directory. If the file does not exist or fails to load, the action is dropped (an error is logged).

### Important notes
- Buttons must be wired as **active-LOW** (pull-up, switch to GND)
- The loop runs every **500 ms** - shorter presses may be missed
- It only reacts to pins configured as inputs in the Config register (M262)
- The TCA6408A INT pin is not used — the latency is given by the polling
- The outputs are never driven automatically — only manually via M264/M265/M266/M267 (e.g. from macro files, start/end G-code etc.)

---

## 7. Persistent Storage

The registers are stored in EEPROM via config_store, so they survive a restart.

### Definition (store_definition.hpp:632-634)

```cpp
StoreItem<uint8_t, 0, ItemFlag::printer_state,
          journal::hash("IO Expander's Configuration Register")>
    io_expander_config_register;

StoreItem<uint8_t, 0, ItemFlag::printer_state,
          journal::hash("IO Expander's Output Register")>
    io_expander_output_register;

StoreItem<uint8_t, 0, ItemFlag::printer_state,
          journal::hash("IO Expander's Polarity Register")>
    io_expander_polarity_register;
```

All three items default to `0` and carry the `ItemFlag::printer_state` flag (they are reset when the "printer state" is erased, e.g. on a factory reset).

### Usage

```cpp
#include <config_store/store_instance.hpp>

// Read
uint8_t config = config_store().io_expander_config_register.get();

// Write (automatically persisted to EEPROM)
config_store().io_expander_output_register.set(0x55);
```

Note: a direct write via `config_store()` does not update the chip — that is what `update_register()` is for, doing both consistently.

---

## 8. Initialization

### Boot sequence (src/buddy/main.cpp:482-485)

```cpp
#if HAS_I2C_EXPANDER()
    // I2C IO Expander have to be initialized after Configuration Store
    buddy::hw::io_expander2.initialize();
#endif
```

The global instance is defined in `src/device/stm32f4/peripherals.cpp:77`:

```cpp
TCA6408A io_expander2(I2C_HANDLE_FOR(io_expander2));
```

### Initialization process (TCA6408A::initialize)

```cpp
void TCA6408A::initialize() {
    is_initialized_ = true;  // Set preliminarily so write_reg works

    // 3 initialization attempts
    for (uint8_t i = 0; i < 3; i++) {
        // Replays the EEPROM values into the chip in the order Output, Config, Polarity
        if (!write_reg(Register::Output,
                       config_store().io_expander_output_register.get()))
            continue;
        if (!write_reg(Register::Config,
                       config_store().io_expander_config_register.get()))
            continue;
        if (!write_reg(Register::Polarity,
                       config_store().io_expander_polarity_register.get()))
            continue;

        return;  // Success
    }
    is_initialized_ = false;  // Failure after 3 attempts
}
```

If the expander is not connected (or the initialization fails), `is_initialized_` stays `false` and all further operations (G-codes, button loop) are inactive — the firmware otherwise runs normally.

---

## 9. I2C communication

### Protocol

**Register write:**
```
START -> ADDR_W (0x40) -> REG_ADDR -> DATA -> STOP
```

**Register read:**
```
START -> ADDR_W (0x40) -> REG_ADDR -> STOP -> [5ms delay] -> START -> ADDR_R (0x41) -> DATA -> STOP
```

### Implementation (TCA6408A.cpp)

```cpp
bool TCA6408A::write_reg(Register reg, uint8_t value) {
    uint8_t data[2] = { (uint8_t)reg, value };
    i2c::Result res = i2c::Transmit(i2c, write_address, data, 2, timeout_ms);
    return res == i2c::Result::ok;
}

bool TCA6408A::read_reg(Register reg, uint8_t &value) const {
    // First send the register address
    i2c::Result res_tx = i2c::Transmit(i2c, write_address,
                                        (uint8_t*)&reg, 1, timeout_ms);
    HAL_Delay(5);  // fixed wait; there is a TODO in the code to shorten it

    // Then read the data
    i2c::Result res_rx = i2c::Receive(i2c, read_address, &value, 1, timeout_ms);

    return res_tx == i2c::Result::ok && res_rx == i2c::Result::ok;
}
```

Every register read therefore blocks for at least 5 ms (`HAL_Delay`). The I2C2 bus is shared by the expander with the EEPROM, the USB-C controller and the "gcode" peripheral (see `include/device/stm32f4/device/peripherals.h`).

---

## 10. Board and printer support

Feature flag in `ProjectOptions.cmake:542`:

```cmake
set_feature_for_printers_master_board(HAS_I2C_EXPANDER "MK3.5" "MK4" "COREONE" "COREONEL")
```

| Printer / board | Supported | Note |
|-----------------|-----------|------|
| MK3.5/S, MK3.9/S, MK4/S (xBuddy) | Yes | The "MK4" build also covers MK3.9 and MK4S |
| Core One, Core One L (xBuddy) | Yes | |
| iX (xBuddy) | No | The board could do it, the flag is not enabled for it |
| XL (XLBuddy) | No | Has `io_expander1` (PCA9557) for internal purposes; `i2c_io_expander2 = -1` |
| Mini (Buddy) | No | `i2c_io_expander2 = -1` |

The mapping to a particular I2C bus is in `include/device/stm32f4/device/peripherals.h` (`i2c_io_expander2`): on xBuddy = bus 2, elsewhere = -1 (not present).

---

## 11. Button wiring diagram

```
GPIO Pin (4-7) ---+--- (internal board pull-up to 3.3 V)
                  |
                  +--- Button --- GND
```

- Wire buttons **only to pins 4-7** (0-3 are not inputs, see chapter 1)
- Idle state: HIGH (~3.3 V via the internal board pull-up)
- Pressed state: LOW (0)
- The firmware inverts the logic (detects LOW as a press)
- The TCA6408A chip itself has no internal pull-ups, but **the board has them fitted on pins 4-7** — no external pull-up is needed
- Switch bigger loads (relays, LED strips, fans) via the power pins 0-3 (open-drain, max. 24 V / 500 mA); pins 4-7 can drive at most 10 mA

---

## 12. Limits of the current implementation

- **Input latency up to 500 ms** — the button loop polling interval; the chip INT pin is unused. (`M269` reads the pin faster, every 100 ms.)
- **Press-edge semantics only** — the button loop does not react to releases nor to holding/levels; waiting for a pin level is handled by `M269`.
- **Reactions only as G-code from USB** — without a flash drive in the printer the macro buttons do not work.
- **No automatic output control** — printer states (errors, warnings, print progress) are not reflected on the pins; outputs must be driven explicitly by G-code.
- **M264/M265/M267 write every change to EEPROM** — for frequent signalling (e.g. on every filament change or timelapse) use the volatile `M266`.
