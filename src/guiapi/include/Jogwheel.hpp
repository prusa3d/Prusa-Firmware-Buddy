/*
 * Jogwheel.hpp
 * \brief Jogwheel input handler
 *
 *  Created on: July 9, 2020
 *      Author: Migi <michal.rudolf23@gmail.com>
 */

#pragma once

#include <inttypes.h>

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
    void Update1ms() volatile;

    /** Returns button input state, this function is for BSOD and situations where interupts are disabled. */
    static int GetJogwheelButtonPinState();

    /** Returns if encoder was changed. */
    bool ConsumeEncoderChanged() volatile {
        bool ret = jogwheel_changed & 2;
        jogwheel_changed = 0;
        return ret;
    }

    enum class ButtonAction {
        NoAction = 0,
        Clicked,
        DoubleClicked,
        Held,
        Pushed,
    };

    /** Returns button action that was triggered, NoAction = 0, BTN_CLICK, BTN_DOUBLE CLICK or Held. */
    ButtonAction ConsumeButtonAction() volatile;

    /** Returns current encoder value. */
    int32_t GetEncoder() const volatile { return encoder; }

    /**
     * Sets type of the jogwheel.
     *
     * There are two types with slightly different behaviour
     * and we have to decide in runtime (guimain.cpp), which type are we using.
     *
     * @param [in] delay - jogwheel type recognition mechanism
     */
    void SetJogwheelType(uint16_t delay) volatile;

    /** Returns difference between last_encoder and encoder and then resets last_encoder */
    int32_t GetEncoderDiff() volatile;

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
    void UpdateVariables(uint8_t signals) volatile;

    /**
     * Updates encoder, different types of jogwheel have different implementations.
     *
     * @param [in] change - change of signals from the last read.
     * @param [in] signals - current signals read from input pins.
     */
    int32_t JogwheelTypeBehaviour(uint8_t change, uint8_t signals) const volatile;

    /**
     * Analyzes member variables and updates btn_action if any button action was triggered.
     *
     * It stores button action until it gets returned in gui_loop, then it is flipped back to NoAction.
     */
    void UpdateButtonAction() volatile;

    /**
     * Switches up encoders gears.
     *
     * This feature is mainly for spinner, to improve encoder's lifespan. Faster spin gives bigger encoder values.
     */
    void Transmission() volatile;

    int32_t encoder;              //!< jogwheel encoder
    int32_t last_encoder;         //!< helping variable - GUI encoder variable for diff counting (sets itself equal to encoder in gui_loop)
    uint16_t doubleclick_counter; //!< keep track of ms from last click
    uint16_t hold_counter;        //!< keep track of ms from button down

    uint8_t jogwheel_signals;     //!< input signals
    uint8_t jogwheel_changed;     //!< stores changed input states
    uint8_t jogwheel_signals_old; //!< stores pre-previous input signals
    uint8_t jogwheel_signals_new; //!< stores previous signals
    ButtonAction btn_action;      //!< variable describes the last button action or NoAction when action was already executed
    bool jogwheel_button_down;    //!< helping variable - stores last button input pin state
    bool btn_pressed;             //!< keeps current state of button input pin
    bool doubleclicked;           //!< helping variable - detect doubleclick after first click's release
    bool being_held;              //!< helping variable - don't click after hold was detected
    bool type1;                   //!< jogwheel is type1 = true or type2 = false
    bool spin_accelerator;        //!< turns up spin accelerator feature
    uint32_t spin_speed_counter;  //!< counting variable for encoder_gear system
    uint32_t speed_traps[4];      //!< stores previous encoder's change timestamp
    uint8_t encoder_gear;         //!< multiple gears for jogwheel spinning
};

extern volatile Jogwheel jogwheel; // Jogwheel static instance
