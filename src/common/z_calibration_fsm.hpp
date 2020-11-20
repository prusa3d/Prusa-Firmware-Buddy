#pragma once

#include "marlin_server.hpp"

class Z_Calib_FSM : public Notifier_POS_Z {
public:
    Z_Calib_FSM(ClientFSM type, uint8_t phase, float min, float max, uint8_t progress_min, uint8_t progress_max);
    ~Z_Calib_FSM();

private:
    bool sw_endstop_state;
    bool hw_endstop_state;
};
