#pragma once
#include <stdint.h>

// Helper macros to parse the operations from Btns()
#define BUTTON_OP_RIGHT(X)  ((X & 0xF0) >> 4)
#define BUTTON_OP_MIDDLE(X) (X & 0x0F)

namespace MMU2 {

/// Button codes + extended actions performed on the printer's side
enum class Buttons : uint_least8_t {
    Right = 0,
    Middle,
    Left,

    // performed on the printer's side
    ResetMMU,
    Load,
    Eject,
    StopPrint,
    DisableMMU,
    TuneMMU, // Printer changes MMU register value

    NoButton = 0xff // shall be kept last
};

constexpr uint_least8_t buttons_to_uint8t(Buttons b) {
    return static_cast<uint8_t>(b);
}

} // namespace MMU2
