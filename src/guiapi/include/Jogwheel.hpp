/*
 * Jogwheel.hpp
 * \brief Jogwheel input handler
 *
 *  Created on: July 9, 2020
 *      Author: Migi <michal.rudolf23@gmail.com>
 */

#pragma once

#include <inttypes.h>
#include "circle_buffer.hpp"

//old encoder (with new encoder 2 steps per 1 count) - Type2
//new encoder (1 steps per 1 count) - Type1

class Jogwheel {
public:
    /**
     * Constructor
     *
     * sets up default values to members
     *
     */
    Jogwheel();
    ~Jogwheel() = default;

    /** Updates jogwheel states and variables every 1ms, this function is called from the interupt. */
    void Update1ms();

    /** Returns button input state, this function is for BSOD and situations where interupts are disabled. */
    static int GetJogwheelButtonPinState();

    // actual state of button, event is stored into buffer on button change
    enum class BtnState_t : uint8_t {
        Released,
        Pressed,
        Held
    };

    /**
     * Fills up the parameter with an event from inner event buffer.
     * Disables interrupt during process
     *
     * returns success (inner buffer was not empty)
     *
     * @param [out] ev - stores an event if inner buffer is not empty
     */
    volatile bool ConsumeButtonEvent(BtnState_t &ev);

    /** Returns current encoder value. */
    volatile int32_t GetEncoder() const { return encoder; }

    /**
     * Sets type of the jogwheel.
     *
     * There are two types with slightly different behaviour
     * and we have to decide in runtime (guimain.cpp), which type are we using.
     *
     * @param [in] delay - jogwheel type recognition mechanism
     */
    void SetJogwheelType(uint16_t delay);

    /** Returns difference between last_encoder and encoder and then resets last_encoder */
    volatile int32_t GetEncoderDiff();

private:
    /**
     * Fills up the parameter with input pins signals.
     *
     * pinENC - button input pin, pinEN1 and pinEN2 - encoder input pins.
     *
     * @param [out] signals - stores signals: bit0 - phase0, bit1 - phase1, bit2 - button pressed (inverted)
     */
    static void ReadInput(uint8_t &signals);

    /**
     * Updates member variables according to input signals.
     *
     * @param [in] signals - input signals
     */
    void UpdateVariables(uint8_t signals);

    /**
     * Updates encoder, different types of jogwheel have different implementations.
     *
     * @param [in] change - change of signals from the last read.
     * @param [in] signals - current signals read from input pins.
     */
    int32_t JogwheelTypeBehaviour(uint8_t change, uint8_t signals) const;

    /**
     * Analyzes member variables and updates btn_event if any button event was triggered.
     *
     * It stores button event into buffer which shall be read in gui_loop.
     */
    void UpdateButtonAction();

    /**
     * Switches up encoders gears.
     *
     * This feature is mainly for spinner, to improve encoder's lifespan. Faster spin gives bigger encoder values.
     */
    void Transmission();

    /** Returns button state (HW pin) from last reading. */
    bool IsBtnPressed();

    /** Changes button state and stores event into buffer */
    void ChangeStateTo(BtnState_t new_state);

    // variables are set in interrupt, volatile variables are read in GUI
    // ordered by size, from biggest to smallest (most size-effective)
    CircleBuffer<BtnState_t, 32> btn_events; //!< event buffer
    uint32_t speed_traps[4];                 //!< stores previous encoder's change timestamp
    uint32_t spin_speed_counter;             //!< counting variable for encoder_gear system
    volatile int32_t encoder;                //!< jogwheel encoder
    volatile int32_t last_encoder;           //!< helping variable - GUI encoder variable for diff counting (sets itself equal to encoder in gui_loop)
    uint16_t hold_counter;                   //!< keep track of ms from button down
    BtnState_t btn_state;                    //!< actual state of button, size uint8_t
    uint8_t jogwheel_signals;                //!< input signals
    uint8_t jogwheel_signals_old;            //!< stores pre-previous input signals
    uint8_t jogwheel_noise_filter;           //!< stores previous signals
    volatile uint8_t encoder_gear;           //!< multiple gears for jogwheel spinning
    bool type1;                              //!< jogwheel is type1 = true or type2 = false
    bool spin_accelerator;                   //!< turns up spin accelerator feature
};

extern Jogwheel jogwheel; // Jogwheel static instance
