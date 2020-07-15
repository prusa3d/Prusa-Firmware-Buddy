// jogwheel.h

#pragma once

#include <inttypes.h>

#define JOGWHEEL_FLG_INV_DIR 0x01
#define JOGWHEEL_FLG_INV_ENC 0x02
#define JOGWHEEL_FLG_INV_E12 0x04
#define JOGWHEEL_FLG_2PULSES 0x08
#define JOGWHEEL_FLG_FILTER2 0x10
//old encoder (with new encoder 2 steps per 1 count)
//#define JOGWHEEL_DEF_FLG      (JOGWHEEL_FLG_INV_ENC | JOGWHEEL_FLG_INV_DIR)
//new encoder (1 steps per 1 count)
#define JOGWHEEL_DEF_FLG (JOGWHEEL_FLG_INV_ENC | JOGWHEEL_FLG_2PULSES | JOGWHEEL_FLG_FILTER2)

typedef struct _jogwheel_config_t {
    uint8_t flg; // flags
} jogwheel_config_t;

extern uint8_t jogwheel_signals;
extern int32_t jogwheel_encoder;
extern uint16_t jogwheel_button_down;
extern uint8_t jogwheel_changed;

void jogwheel_update_1ms(void);
void jogwheel_encoder_set(int32_t val, int32_t min, int32_t max);
bool jogwheel_low_level_button_pressed();

extern jogwheel_config_t jogwheel_config;
