#include <limits.h>

#include "Jogwheel.hpp"
#include "hwio_pindef.h"
#include "cmsis_os.h" //__disable_irq, __enabe_irq, HAL_GetTick

using buddy::hw::jogWheelEN1;
using buddy::hw::jogWheelEN2;
using buddy::hw::jogWheelENC;
using buddy::hw::Pin;

// time constants
static const constexpr uint16_t JG_HOLD_INTERVAL = 1000; // [ms] jogwheel min hold delay - if user holds for shorter time than this, it triggers normal click instead

// encoder limits
// used later to compare with int, warning does not do anything - no 32bit integer is bigger than INT_MAX or lower than INT_MIN
static const constexpr int32_t JG_ENCODER_MAX = INT_MAX;
static const constexpr int32_t JG_ENCODER_MIN = INT_MIN;

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

Jogwheel::Jogwheel()
    : speed_traps { 0, 0, 0, 0 }
    , spin_speed_counter(0)
    , encoder(0)
    , hold_counter(0)
    , btn_state(BtnState_t::Released)
    , jogwheel_signals(0)
    , jogwheel_signals_old(0)
    , jogwheel_noise_filter(0)
    , encoder_gear(1)
    , type1(true)
    , spin_accelerator(false) {
}

int Jogwheel::GetJogwheelButtonPinState() {
    return static_cast<int>(jogWheelENC.read());
}

void Jogwheel::ReadInput(uint8_t &signals) {

    if (jogWheelENC.read() == Pin::State::high) {
        signals |= JG_BUTTON_PRESSED; //bit 2 - button press
    }
    signals ^= JG_BUTTON_PRESSED; // we are using inverted button pin

    if (jogWheelEN1.read() == Pin::State::high) {
        signals |= JG_PHASE_0; //bit 0 - phase0
    }
    if (jogWheelEN2.read() == Pin::State::high) {
        signals |= JG_PHASE_1; //bit 1 - phase1
    }
}

volatile bool Jogwheel::ConsumeButtonEvent(Jogwheel::BtnState_t &ev) {
    static uint32_t last_read;          //cannot read it too often
    const uint32_t min_read_dellay = 8; //ms
    if ((last_read - HAL_GetTick()) < min_read_dellay) {
        return false;
    }
    __disable_irq();
    volatile bool ret = btn_events.ConsumeFirst(ev);
    __enable_irq();
    return ret;
}

void Jogwheel::SetJogwheelType(uint16_t delay) {
    type1 = delay > 1000;
}

void Jogwheel::UpdateButtonAction() {
    switch (btn_state) {
    case BtnState_t::Released:
        if (IsBtnPressed()) {
            ChangeStateTo(BtnState_t::Pressed);
            hold_counter = 0;
        }
        break;
    case BtnState_t::Pressed:
        if (!IsBtnPressed()) {
            ChangeStateTo(BtnState_t::Released);
        } else {
            if ((++hold_counter) > JG_HOLD_INTERVAL) {
                ChangeStateTo(BtnState_t::Held);
            }
        }
        break;
    case BtnState_t::Held:
        if (!IsBtnPressed()) {
            ChangeStateTo(BtnState_t::Released);
        }
        break;
    }
}

bool Jogwheel::IsBtnPressed() {
    return (jogwheel_signals & JG_BUTTON_PRESSED) != 0;
}

void Jogwheel::ChangeStateTo(BtnState_t new_state) {
    btn_state = new_state;
    btn_events.push_back_DontRewrite(new_state);
}

volatile int32_t Jogwheel::GetEncoderDiff() {
    static uint32_t last_read;           //cannot read it too often
    const uint32_t min_read_dellay = 32; //ms
    if ((last_read - HAL_GetTick()) < min_read_dellay) {
        return 0;
    }

    static int32_t last_encoder = 0;
    __disable_irq();
    volatile int32_t diff = encoder - last_encoder;
    last_encoder = encoder;
    diff *= encoder_gear;
    __enable_irq();
    return diff;
}

void Jogwheel::Update1ms() {
    spin_speed_counter++;

    uint8_t signals = 0;

    ReadInput(signals);

    if (jogwheel_noise_filter != signals) {
        jogwheel_noise_filter = signals; // noise detection
        return;
    }

    UpdateVariables(signals);

    UpdateButtonAction();
}

int32_t Jogwheel::JogwheelTypeBehaviour(uint8_t change, uint8_t signals) const {
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

void Jogwheel::UpdateVariables(uint8_t signals) {
    uint8_t change = signals ^ jogwheel_signals;

    if (change & JG_PHASES_CHANGED) //encoder phase signals changed
    {
        int32_t new_encoder = JogwheelTypeBehaviour(change, signals); // derived function - different types of jogwheel

        if (encoder < JG_ENCODER_MIN)
            encoder = JG_ENCODER_MIN;
        if (encoder > JG_ENCODER_MAX)
            encoder = JG_ENCODER_MAX;
        if (encoder != new_encoder) {
            encoder = new_encoder;
            change |= JG_ENCODER_CHANGED; //bit3 means encoder changed
            speed_traps[3] = speed_traps[2];
            speed_traps[2] = speed_traps[1];
            speed_traps[1] = speed_traps[0];
            speed_traps[0] = spin_speed_counter;
            Transmission();
        }
    }

    if (change & (JG_PHASES_OR_BUTTON_CHANGED | JG_ENCODER_CHANGED)) //encoder phase signals, encoder or button changed
    {
        jogwheel_signals_old = jogwheel_signals; //save old signal state
        jogwheel_signals = signals;              //update signal state
    }
}

//if encoder is not moved 49 days, this will fail
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
