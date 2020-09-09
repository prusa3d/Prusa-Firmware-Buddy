#include <limits.h>

#include "Jogwheel.hpp"
#include "guiconfig.h"
#include "Pin.hpp"

#ifdef GUI_JOGWHEEL_SUPPORT

// time constants
static const constexpr uint16_t JG_DOUBLECLICK_INTERVAL = 500; // [ms] jogwheel max double click delay - if second click is after 500ms it doesn't trigger doubleclick
static const constexpr uint16_t JG_HOLD_INTERVAL = 1000;       // [ms] jogwheel min hold delay - if user holds for shorter time than this, it triggers normal click instead

// encoder limits
static const constexpr int32_t JG_ENCODER_MAX = INT_MAX;
static const constexpr int32_t JG_ENCODER_MIN = INT_MIN;

static InputPin jogWheelEN1(IoPort::E, IoPin::p15, IMode::input, Pull::up);
static InputPin jogWheelEN2(IoPort::E, IoPin::p13, IMode::input, Pull::up);
static InputPin jogWheelENC(IoPort::E, IoPin::p12, IMode::input, Pull::up);

// signal flags
enum : uint8_t {
    JG_PHASE_0 = 0x01,
    JG_PHASE_1 = 0x02,
    JG_PHASES_CHANGED = 0x03,
    JG_BUTTON_PRESSED = 0x04,
    JG_PHASES_OR_BUTTON_CHANGED = 0x07,
    JG_ENCODER_CHANGED = 0x08,
    JG_BUTTON_OR_ENCODER_CHANGED = 0x0C,
};

Jogwheel::Jogwheel() {
    jogwheel_signals_old = jogwheel_signals_new = jogwheel_signals = last_encoder = encoder = jogwheel_changed = doubleclick_counter = hold_counter = spin_speed_counter = 0;
    encoder_gear = 1;

    speed_traps[0] = speed_traps[1] = speed_traps[2] = speed_traps[3] = 0;
    btn_pressed = doubleclicked = being_held = jogwheel_button_down = false;
    btn_action = ButtonAction::BTN_NO_ACTION;
    type1 = true;
    spin_accelerator = false;
}

const int Jogwheel::GetJogwheelButtonPinState() const {
    return jogWheelENC.read();
}

void Jogwheel::ReadInput(uint8_t &signals) {

    if (jogWheelENC.read()) {
        signals |= JG_BUTTON_PRESSED; //bit 2 - button press
    }
    signals ^= JG_BUTTON_PRESSED; // we are using inverted button pin

    if (jogWheelEN1.read()) {
        signals |= JG_PHASE_0; //bit 0 - phase0
    }
    if (jogWheelEN2.read()) {
        signals |= JG_PHASE_1; //bit 1 - phase1
    }
}

const Jogwheel::ButtonAction Jogwheel::GetButtonAction() {
    ButtonAction ret = btn_action;
    btn_action = ButtonAction::BTN_NO_ACTION;
    return ret;
}

void Jogwheel::SetJogwheelType(uint16_t delay) {
    type1 = delay > 1000;
}

void Jogwheel::UpdateButtonAction() {
    if (!btn_pressed && jogwheel_button_down) {
        btn_action = ButtonAction::BTN_PUSHED;
        btn_pressed = true;
        hold_counter = 1;
        if (doubleclick_counter > 0) { // double click detection interval goes <click release;second click push>
            doubleclicked = true;
            doubleclick_counter = 0;
        }
    } else if (btn_pressed && !jogwheel_button_down) {
        btn_pressed = false;
        if (being_held) {
            being_held = false;
        } else {
            if (doubleclicked) {
                btn_action = ButtonAction::BTN_DOUBLE_CLICKED;
                doubleclick_counter = 0;
                doubleclicked = false;
            } else {
                btn_action = ButtonAction::BTN_CLICKED;
                doubleclick_counter = 1;
            }
        }
        hold_counter = 0;
    }

    if (doubleclick_counter) {
        doubleclick_counter++;
        if (doubleclick_counter > JG_DOUBLECLICK_INTERVAL) {
            doubleclick_counter = 0;
        }
    }
    if (hold_counter) {
        hold_counter++;
        if (hold_counter > JG_HOLD_INTERVAL) {
            btn_action = ButtonAction::BTN_HELD;
            doubleclick_counter = 0;
            hold_counter = 0;
            being_held = true;
        }
    }
}

int32_t Jogwheel::GetEncoderDiff() {
    int32_t diff = encoder - last_encoder;
    last_encoder = encoder;
    // WARNING: jogwheel_button_down was here
    return diff * encoder_gear;
}

void Jogwheel::Update1ms() {

    uint8_t signals = 0;

    ReadInput(signals);

    UpdateVariables(signals);

    UpdateButtonAction();
}

int32_t Jogwheel::JogwheelTypeBehaviour(const uint8_t change, const uint8_t signals) const {
    int32_t new_encoder = encoder;
    if (type1) {
        if ((change & JG_PHASE_0) && (signals & JG_PHASE_0) && !(signals & JG_PHASE_1))
            new_encoder--;
        if ((change & JG_PHASE_1) && (signals & JG_PHASE_1) && !(signals & JG_PHASE_0))
            new_encoder++;
    } else {
        uint8_t prev_change = jogwheel_signals ^ jogwheel_signals_old;
        if (((signals & JG_PHASES_CHANGED) == 0) || ((signals & JG_PHASES_CHANGED) == JG_PHASES_CHANGED)) {
            if (((change & JG_PHASES_CHANGED) == JG_PHASE_0) && ((prev_change & JG_PHASES_CHANGED) == JG_PHASE_1))
                new_encoder++;
            if (((change & JG_PHASES_CHANGED) == JG_PHASE_1) && ((prev_change & JG_PHASES_CHANGED) == JG_PHASE_0))
                new_encoder--;
        }
    }
    return new_encoder;
}

void Jogwheel::UpdateVariables(const uint8_t signals) {

    spin_speed_counter++;

    if (jogwheel_signals_new != signals) {
        jogwheel_signals_new = signals; // noise detection
        return;
    }

    uint8_t change = signals ^ jogwheel_signals;

    if (change & JG_PHASES_CHANGED) //encoder phase signals changed
    {
        int32_t new_encoder = JogwheelTypeBehaviour(change, signals); // derived function - different types of jogwheel

        if (encoder < JG_ENCODER_MIN)
            encoder = JG_ENCODER_MIN;
        if (encoder > JG_ENCODER_MAX)
            encoder = JG_ENCODER_MAX;
        if (encoder != new_encoder) { // last_encoder is not used because it stores GUI last encoder position
            encoder = new_encoder;
            change |= JG_ENCODER_CHANGED; //bit3 means encoder changed
            speed_traps[3] = speed_traps[2];
            speed_traps[2] = speed_traps[1];
            speed_traps[1] = speed_traps[0];
            speed_traps[0] = spin_speed_counter;
            Transmission();
        }
    }

    jogwheel_button_down = signals & JG_BUTTON_PRESSED;

    if (change & JG_PHASES_OR_BUTTON_CHANGED) //encoder phase signals or button changed
    {
        jogwheel_signals_old = jogwheel_signals; //save old signal state
        jogwheel_signals = signals;              //update signal state
    }
    if (change & JG_BUTTON_OR_ENCODER_CHANGED) //encoder changed or button changed
    {
        jogwheel_signals_old = jogwheel_signals; //save old signal state
        jogwheel_signals = signals;              //update signal state
        jogwheel_changed |= (change >> 2);       //synchronization is not necessary because we are inside interrupt
    }
}

void Jogwheel::Transmission() {
    uint32_t time_diff = speed_traps[0] - speed_traps[1];
    time_diff = time_diff > speed_traps[1] - speed_traps[2] ? time_diff : speed_traps[1] - speed_traps[2];
    time_diff = time_diff > speed_traps[2] - speed_traps[3] ? time_diff : speed_traps[2] - speed_traps[3];

    if (time_diff > 25 || time_diff == 0) {
        encoder_gear = 1;
    } else if (time_diff > 10) {
        encoder_gear = 3;
    } else {
        encoder_gear = 5;
    }
}
#endif //GUI_JOGWHEEL_SUPPORT
