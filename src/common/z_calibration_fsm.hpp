/*
 * z_calibration_fsm.hpp
 *
 * Finite State Machine for Z calibration
 *
 *  Created on: Jul 22, 2020
 *      Author: Migi <michal.rudolf23[at]gmail.com>
 */

#pragma once

#include "marlin_server.hpp"

/**
 * Z_Calib_FSM
 *
 * Derived Finite state machine class for smooth progress of Z calibration
 */
class Z_Calib_FSM : public marlin_server::FSM_notifier {
public:
    /**
     * Constructor
     * Saves current state of HW and SW endstop and change them for calibration purposes.
     * @param [in] type - type of FSM
     * @param [in] phase - current state in FSM
     * @param [in] min - current parameter value for progress mapping
     * @param [in] max - target value for progress mapping
     * @param [in] progress_min - start progress value
     * @param [in] progress_max - end progress value
     */
    Z_Calib_FSM(ClientFSM type, uint8_t phase, float min, float max, uint8_t progress_min, uint8_t progress_max);

    /**
     * Destructor
     * Sets back stored states of HW and SW endstop.
     */
    ~Z_Calib_FSM();

    /** Disables HW endstops - for aligning both Z axes to on the same Z position */
    void Disable_Stallguard();
    virtual fsm::PhaseData serialize(uint8_t progress) override;

private:
    bool sw_endstop_state; //<! store variable for SW endstop state
    bool hw_endstop_state; //<! store variable for HW endstop state
};
