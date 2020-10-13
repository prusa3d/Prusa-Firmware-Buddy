#include <limits.h>

#include "Jogwheel.hpp"
#include "hwio_pindef.h"
#include "cmsis_os.h" //__disable_irq, __enabe_irq, HAL_GetTick
#include "queue.h"    // freertos queue
#include "bsod.h"

using SpinMessage_t = int32_t;

// rtos queues config
static constexpr size_t ButtonMessageQueueLength = 32;
static constexpr size_t SpinMessageQueueLength = 32;
static constexpr size_t SpinSendPeriodMs = 32;

#if (configSUPPORT_STATIC_ALLOCATION == 1)

void Jogwheel::InitButtonMessageQueueInstance_NotFromISR() {
    constexpr size_t item_sz = sizeof(Jogwheel::BtnState_t);
    constexpr size_t length = ButtonMessageQueueLength;
    static StaticQueue_t queue;
    static uint8_t storage_area[length * item_sz];
    button_queue_handle = xQueueCreateStatic(length, item_sz, storage_area, &queue);
}

void Jogwheel::InitSpinMessageQueueInstance_NotFromISR() {
    constexpr size_t item_sz = sizeof(SpinMessage_t);
    constexpr size_t length = SpinMessageQueueLength;
    static StaticQueue_t queue;
    static uint8_t storage_area[length * item_sz];
    spin_queue_handle = xQueueCreateStatic(length, item_sz, storage_area, &queue);
}

#else // (configSUPPORT_STATIC_ALLOCATION == 1 )

// QueueHandle_t is a pointer
// cannot be called in interrupt !!!
// could set nullptr, handled in gui thread while reading (bsod)
void Jogwheel::InitButtonMessageQueueInstance_NotFromISR() {
    button_queue_handle = xQueueCreate(ButtonMessageQueueLength, sizeof(Jogwheel::BtnState_t));
}

// QueueHandle_t is a pointer
// cannot be called in interrupt !!!
// could set nullptr, handled in gui thread while reading (bsod)
void Jogwheel::InitSpinMessageQueueInstance_NotFromISR() {
    spin_queue_handle = xQueueCreate(SpinMessageQueueLength, sizeof(SpinMessage_t));
}

#endif // (configSUPPORT_STATIC_ALLOCATION == 1 )

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
    , button_queue_handle(nullptr)
    , spin_queue_handle(nullptr)
    , tick_counter(0)
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

bool Jogwheel::ConsumeButtonEvent(Jogwheel::BtnState_t &ev) {
    // this can happen only once
    // queue is initialized in GUI on first attempt to read
    if (button_queue_handle == nullptr) {
        __disable_irq();
        InitButtonMessageQueueInstance_NotFromISR();
        __enable_irq();

        if (button_queue_handle == nullptr) {
            bsod("ButtonMessageQueue heap malloc error ");
        }
    }

    return (xQueueReceive(button_queue_handle, &ev, 0 /*0 == do not wait*/) == pdPASS);
}

int32_t Jogwheel::ConsumeEncoderDiff() {
    // this can happen only once
    // queue is initialized in GUI on first attempt to read
    if (spin_queue_handle == nullptr) {
        __disable_irq();
        InitSpinMessageQueueInstance_NotFromISR();
        __enable_irq();

        if (spin_queue_handle == nullptr) {
            bsod("SpinMessageQueue heap malloc error ");
        }
    }

    SpinMessage_t ret;

    if (xQueueReceive(spin_queue_handle, &ret, 0 /*0 == do not wait*/) != pdPASS) {
        ret = 0;
    }

    return ret;
}

void Jogwheel::SetJogwheelType(uint16_t delay) {
    type1 = delay > 1000;
}

void Jogwheel::UpdateButtonActionFromISR() {
    switch (btn_state) {
    case BtnState_t::Released:
        if (IsBtnPressed()) {
            ChangeStateFromISR(BtnState_t::Pressed);
            hold_counter = 0;
        }
        break;
    case BtnState_t::Pressed:
        if (!IsBtnPressed()) {
            ChangeStateFromISR(BtnState_t::Released);
        } else {
            if ((++hold_counter) > JG_HOLD_INTERVAL) {
                ChangeStateFromISR(BtnState_t::Held);
            }
        }
        break;
    case BtnState_t::Held:
        if (!IsBtnPressed()) {
            ChangeStateFromISR(BtnState_t::Released);
        }
        break;
    }
}

bool Jogwheel::IsBtnPressed() {
    return (jogwheel_signals & JG_BUTTON_PRESSED) != 0;
}

void Jogwheel::ChangeStateFromISR(BtnState_t new_state) {
    if (button_queue_handle == nullptr)
        return;
    btn_state = new_state;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE; //xQueueSendFromISR require address of this
    xQueueSendFromISR(button_queue_handle, &btn_state, &xHigherPriorityTaskWoken);
}

void Jogwheel::SendEncoderDiffFromISR() {
    if (spin_queue_handle == nullptr)
        return;
    static int32_t last_encoder = 0;
    SpinMessage_t diff = encoder - last_encoder;

    if (diff) {
        last_encoder = encoder;
        diff *= encoder_gear;
        if (diff) {                                        //check again, diff *= encoder_gear could overflow to zero
            BaseType_t xHigherPriorityTaskWoken = pdFALSE; //xQueueSendFromISR require address of this
            xQueueSendFromISR(spin_queue_handle, &diff, &xHigherPriorityTaskWoken);
        }
    }
}

void Jogwheel::Update1msFromISR() {
    //do nothing while queues are not initialized
    if ((button_queue_handle == nullptr) || (spin_queue_handle == nullptr))
        return;
    tick_counter++;

    uint8_t signals = 0;

    ReadInput(signals);

    if (jogwheel_noise_filter != signals) {
        jogwheel_noise_filter = signals; // noise detection
        return;
    }

    UpdateVariablesFromISR(signals);

    UpdateButtonActionFromISR();

    if ((tick_counter % SpinSendPeriodMs) == 0) {
        SendEncoderDiffFromISR();
    }
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

void Jogwheel::UpdateVariablesFromISR(uint8_t signals) {
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
            speed_traps[0] = tick_counter;
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
