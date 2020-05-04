// jogwheel.c

#include "jogwheel.h"
#include <limits.h>
#include "gpio.h"

uint8_t jogwheel_signals = 0;
uint8_t jogwheel_signals_old = 0;
uint8_t jogwheel_signals_new = 0;
int32_t jogwheel_encoder = 0;
int32_t jogwheel_encoder_min = INT_MIN;
int32_t jogwheel_encoder_max = INT_MAX;
uint16_t jogwheel_button_down = 0;
uint8_t jogwheel_changed = 0;

void jogwheel_init(void) {
    gpio_init(jogwheel_config.pinEN1, GPIO_MODE_INPUT, GPIO_PULLUP, GPIO_SPEED_FREQ_LOW);
    gpio_init(jogwheel_config.pinEN2, GPIO_MODE_INPUT, GPIO_PULLUP, GPIO_SPEED_FREQ_LOW);
    gpio_init(jogwheel_config.pinENC, GPIO_MODE_INPUT, GPIO_PULLUP, GPIO_SPEED_FREQ_LOW);
}

void jogwheel_update_1ms(void) {
    uint8_t signals = 0;
    if (gpio_get(jogwheel_config.pinENC))
        signals |= 4; //bit 2 - button press
    if (jogwheel_config.flg & JOGWHEEL_FLG_INV_ENC)
        signals ^= 4;
    if (gpio_get(jogwheel_config.pinEN1))
        signals |= 1; //bit 0 - phase0
    if (gpio_get(jogwheel_config.pinEN2))
        signals |= 2; //bit 1 - phase1
    if (jogwheel_config.flg & JOGWHEEL_FLG_INV_E12)
        signals ^= 3;
    if (jogwheel_config.flg & JOGWHEEL_FLG_FILTER2)
        if (jogwheel_signals_new != signals) {
            jogwheel_signals_new = signals;
            return;
        }
    uint8_t change = signals ^ jogwheel_signals;
    if (change & 3) //encoder phase signals changed
    {
        uint8_t change2 = jogwheel_signals ^ jogwheel_signals_old;
        int32_t encoder = jogwheel_encoder;
        int32_t dir = (jogwheel_config.flg & JOGWHEEL_FLG_INV_DIR) ? -1 : 1;
        if ((jogwheel_config.flg & JOGWHEEL_FLG_2PULSES)) {
            if (((signals & 3) == 0) || ((signals & 3) == 3)) {
                if (((change & 3) == 1) && ((change2 & 3) == 2))
                    encoder += dir;
                if (((change & 3) == 2) && ((change2 & 3) == 1))
                    encoder -= dir;
            }
        } else {
            if ((change & 1) && (signals & 1) && !(signals & 2))
                encoder += dir;
            if ((change & 2) && (signals & 2) && !(signals & 1))
                encoder -= dir;
        }
        if (encoder < jogwheel_encoder_min)
            encoder = jogwheel_encoder_min;
        if (encoder > jogwheel_encoder_max)
            encoder = jogwheel_encoder_max;
        if (jogwheel_encoder != encoder) {
            jogwheel_encoder = encoder;
            change |= 8; //bit3 means encoder changed
        }
    }
    if (signals & 4)
        jogwheel_button_down++;
    else
        jogwheel_button_down = 0;
    if (change & 7) //encoder phase signals or button changed
    {
        jogwheel_signals_old = jogwheel_signals; //save old signal state
        jogwheel_signals = signals;              //update signal state
    }
    if (change & 12) //encoder changed or button changed
    {
        jogwheel_signals_old = jogwheel_signals; //save old signal state
        jogwheel_signals = signals;              //update signal state
        jogwheel_changed |= (change >> 2);       //synchronization is not necessary because we are inside interrupt
    }
}

void jogwheel_encoder_set(int32_t val, int32_t min, int32_t max) {
    if (min > max)
        return;
    if (val < min)
        val = min;
    if (val > max)
        val = max;
    jogwheel_encoder = val;
    jogwheel_encoder_min = min;
    jogwheel_encoder_max = max;
}

jogwheel_config_t jogwheel_config = {
    0, // encoder phase1 pin
    0, // encoder phase2 pin
    0, // button pin
    0, // flags
};
