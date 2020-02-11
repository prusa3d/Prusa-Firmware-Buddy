// jogwheel.h

#ifndef _JOGWHEEL_H
#define _JOGWHEEL_H

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

#pragma pack(push)
#pragma pack(1)

typedef struct _jogwheel_config_t {
    uint8_t pinEN1; // encoder phase1 pin
    uint8_t pinEN2; // encoder phase2 pin
    uint8_t pinENC; // button pin
    uint8_t flg; // flags
} jogwheel_config_t;

#pragma pack(pop)

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern uint8_t jogwheel_signals;
extern int32_t jogwheel_encoder;
extern uint16_t jogwheel_button_down;
extern uint8_t jogwheel_changed;

extern void jogwheel_init(void);

extern void jogwheel_update_1ms(void);

extern void jogwheel_encoder_set(int32_t val, int32_t min, int32_t max);

extern jogwheel_config_t jogwheel_config;

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // _JOGWHEEL_H
