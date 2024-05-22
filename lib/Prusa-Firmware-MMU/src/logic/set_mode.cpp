/// @file set_mode.cpp
#include "set_mode.h"
#include "../modules/globals.h"
#include "../modules/motion.h"

namespace logic {

SetMode setMode;

bool SetMode::Reset(uint8_t param) {
    mg::globals.SetMotorsMode(param != 0); // remember the last mode set
    // distribute the mode to all motors immediately
    mm::motion.SetMode((param == 0) ? mm::Normal : mm::Stealth);
    FinishedOK();
    return true;
}

} // namespace logic
