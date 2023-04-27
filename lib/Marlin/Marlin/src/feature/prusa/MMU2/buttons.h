#pragma once
#include <stdint.h>

// Helper macros to parse the operations from Btns()
#define BUTTON_OP_RIGHT(X)  ((X & 0xF0) >> 4)
#define BUTTON_OP_MIDDLE(X) (X & 0x0F)

namespace MMU2 {

/// Button codes + extended actions performed on the printer's side
enum Buttons : uint8_t {
    Right = 0,
    Middle,
    Left,

    // performed on the printer's side
    RestartMMU,
    StopPrint,
    DisableMMU,

    NoButton = 0xff // shall be kept last
};

} // namespace MMU2
