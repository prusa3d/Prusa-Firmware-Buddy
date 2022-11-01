#include "z_calibration_fsm.hpp"
#include "../Marlin/src/module/endstops.h"
#include "../Marlin/src/module/motion.h"

// Z Calibration FSM Notifier

Z_Calib_FSM::Z_Calib_FSM(ClientFSM type, uint8_t phase, float min, float max, uint8_t progress_min, uint8_t progress_max)
    : Notifier(type, phase, min, max, progress_min, progress_max) {
    sw_endstop_state = soft_endstop._enabled;
    hw_endstop_state = endstops.global_enabled();
    if (sw_endstop_state) {
        soft_endstop._enabled = false;
    }
    if (!hw_endstop_state) {
        endstops.enable_globally(true);
    }
}
Z_Calib_FSM::~Z_Calib_FSM() {
    soft_endstop._enabled = sw_endstop_state;
    endstops.enable_globally(hw_endstop_state);
}

void Z_Calib_FSM::Disable_Stallguard() {
    endstops.enable_globally(false);
}
