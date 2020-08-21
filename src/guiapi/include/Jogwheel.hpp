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
     * sets up default values to members and initialize input pins
     *
     * @param [in] encoder_pin1 - input pin for encoder phase0
     *
     * @param [in] encoder_pin2 - input pin for encoder phase1
     *
     * @param [in] btn_pin - input pin for button press action
     */
    Jogwheel(uint8_t encoder_pin1, uint8_t encoder_pin2, uint8_t btn_pin);
    ~Jogwheel() {}

    /** Updates jogwheel states and variables every 1ms, this function is called from the interupt. */
    void Update1ms();

    /** Returns button input state, this funtion is for BSOD and situations where interupts are disabled. */
    const int GetJogwheelButtonPinState() const;

    /** Returns if encoder was changed. */
    const bool EncoderChanged() {
        bool ret = jogwheel_changed & 2;
        jogwheel_changed = 0;
        return ret;
    }

    enum class ButtonAction {
        BTN_NO_ACTION = 0,
        BTN_CLICKED,
        BTN_DOUBLE_CLICKED,
        BTN_HELD,
        BTN_PUSHED,
    };

    /** Returns button action that was triggered, BTN_NO_ACTION = 0, BTN_CLICK, BTN_DOUBLE CLICK or BTN_HELD. */
    const ButtonAction GetButtonAction();

    /** Returns current encoder value. */
    const int32_t GetEncoder() const { return encoder; }

    /**
     * Sets type of the jogwheel.
     *
     * There are two types with slightly different behaviour
     * and we have to decide in runtime (guimain.cpp), which type are we using.
     *
     * @param [in] type - jogwheel type
     */
    void SetJogwheelType(const bool type) { type1 = type; }

    /** Returns difference between last_encoder and encoder and then resets last_encoder */
    int32_t GetEncoderDiff();

private:
    /**
     * Fills up the parameter with input pins signals.
     *
     * pinENC - button input pin, pinEN1 and pinEN2 - encoder input pins.
     *
     * @param [out] signals - stores signals: bit0 - phase0, bit1 - phase1, bit2 - button pressed (inverted)
     */
    void ReadInput(uint8_t &signals);

    /**
     * Updates member variables according to input signals.
     *
     * @param [in] signals - input signals
     */
    void UpdateVariables(const uint8_t signals);

    /**
     * Updates encoder, different types of jogwheel have different implementations.
     *
     * @param [in] change - change of signals from the last read.
     * @param [in] signals - current signals read from input pins.
     */
    int32_t JogwheelTypeBehaviour(const uint8_t change, const uint8_t signals) const;

    /**
     * Analyzes member variables and updates btn_action if any button action was triggered.
     *
     * It stores button action until it gets returned in gui_loop, then it is fliped back to BTN_NO_ACTION.
     */
    void UpdateButtonAction();

    /**
     * Switches up encoders gears.
     *
     * This feature is mainly for spinner, to improve encoder's lifespan. Faster spin gives bigger encoder values.
     */
    void Transmission();

    typedef struct {
        uint8_t pinEN1; // encoder phase1 pin
        uint8_t pinEN2; // encoder phase2 pin
        uint8_t pinENC; // button pin
    } Config;

    int32_t encoder;              //!< jogwheel encoder
    int32_t last_encoder;         //!< helping variable - GUI encoder variable for diff counting (sets itself equal to encoder in gui_loop)
    uint16_t doubleclick_counter; //!< keep track of ms from last click
    uint16_t hold_counter;        //!< keep track of ms from button down

    Config config;                //!< pin configurations
    uint8_t jogwheel_signals;     //!< input signals
    uint8_t jogwheel_changed;     //!< stores changed input states
    uint8_t jogwheel_signals_old; //!< stores pre-previous input signals
    uint8_t jogwheel_signals_new; //!< stores previous signals
    ButtonAction btn_action;      //!< variable describes the last button action or BTN_NO_ACTION when action was already executed
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

extern Jogwheel jogwheel; // Jogwheel static instance
