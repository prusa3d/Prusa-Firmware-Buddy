/*
 * Jogwheel.hpp
 * \brief Jogwheel input handler
 *
 *  Created on: July 9, 2020
 *      Author: Migi <michal.rudolf23@gmail.com>
 */

#pragma once

#include <inttypes.h>
#include "window_types.hpp" // BtnState_t

// old encoder (with new encoder 2 steps per 1 count) - Type2
// new encoder (1 steps per 1 count) - Type1

class Jogwheel {
    using QueueHandle_t = void *; // do not want to include rtos files in this header
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
    void Update1msFromISR();

    /** Returns button input state, this function is for BSOD and situations where interupts are disabled. */
    static int GetJogwheelButtonPinState();

    // structure to be read in rtos thread (outside interrupt)
    // size must be 32 bit to be atomic
    struct encoder_t {
        union {
            struct {
                int16_t value;
                uint8_t gear;
                uint8_t tick;
            };
            uint32_t data;
        };
    };

    /**
     * Fills up the parameter with an event from inner event buffer.
     *
     * throws bsod on buffer malloc failure
     *
     * returns success (inner buffer was not empty)
     *
     * @param [out] ev - stores an event if inner buffer is not empty else left unchanged
     */
    bool ConsumeButtonEvent(BtnState_t &ev);

    /** Returns current encoder value, reading int is atomic - can do it directly */
    int32_t GetEncoder() const { return encoder; }

    /**
     * Sets type of the jogwheel.
     *
     * There are two types with slightly different behaviour
     * and we have to decide in runtime (guimain.cpp), which type are we using.
     *
     * @param [in] delay - jogwheel type recognition mechanism
     */
    void SetJogwheelType(uint16_t delay);

    /**
     * Returns difference between last_encoder and encoder and then resets last_encoder
     *
     * throws bsod on buffer malloc failure
     */
    int32_t ConsumeEncoderDiff();

private:
    static int32_t CalculateEncoderDiff(encoder_t enc);

    /**
     * Initialize queue for button messages (button_queue_handle)
     *
     * cannot use Mayers singleton - first call in IRQ can cause deadlock
     *
     * could set nullptr, handled in ConsumeButtonEvent (bsod)
     */
    void InitButtonMessageQueueInstance_NotFromISR();

    /**
     * Initialize queue for button messages (spin_queue_handle)
     *
     * cannot use Mayers singleton - first call in IRQ can cause deadlock
     *
     * could set nullptr, handled in ConsumeEncoderDiff (bsod)
     */
    void InitSpinMessageQueueInstance_NotFromISR();

    /**
     * Converts pin levels to signals variable
     *
     * pinENC - button input pin, pinEN1 and pinEN2 - encoder input pins.
     *
     * returns signals - stores signals: bit0 - phase0, bit1 - phase1, bit2 - button pressed (inverted)
     */
    static uint8_t ReadHwInputsFromISR();

    /**
     * Updates member variables according to input signals.
     *
     * To be used in interrupt
     *
     * @param [in] signals - input signals
     */
    void UpdateVariablesFromISR(uint8_t signals);

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
     * To be used in interrupt
     *
     * It stores button event into rtos queue which shall be read in rtos thread (outside interrupt).
     */
    void UpdateButtonActionFromISR();

    /**
     * Switches up encoders gears.
     *
     * This feature is mainly for spinner, to improve encoder's lifespan. Faster spin gives bigger encoder values.
     */
    void Transmission();

    /** Returns button state (HW pin) from last reading. */
    bool IsBtnPressed();

    /** Changes button state and stores event into buffer */
    void ChangeStateFromISR(BtnState_t new_state);

    // variables are set in interrupt
    // ordered by size, from biggest to smallest (most size-effective)
    uint32_t speed_traps[4]; //!< stores previous encoder's change timestamp
    QueueHandle_t button_queue_handle; //!< pointer to message button queue, cannot use Mayers singleton - first call in IRQ can cause deadlock
    volatile encoder_t threadsafe_enc; //!< encoder data struct to be passed to rtos thread (outside interrupt)
    uint32_t tick_counter; //!< counting variable for encoder_gear system
    int32_t encoder; //!< jogwheel encoder
    uint16_t hold_counter; //!< keep track of ms from button down
    BtnState_t btn_state; //!< current state of button, size uint8_t
    uint8_t jogwheel_signals; //!< input signals
    uint8_t jogwheel_signals_old; //!< stores pre-previous input signals
    uint8_t encoder_gear; //!< multiple gears for jogwheel spinning
    bool type1; //!< jogwheel is type1 = true or type2 = false
    bool spin_accelerator; //!< turns up spin accelerator feature
};

extern Jogwheel jogwheel; // Jogwheel static instance
