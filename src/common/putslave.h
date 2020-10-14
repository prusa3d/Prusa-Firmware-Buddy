// putslave.h
#pragma once

#include "uartslave.h"

enum {
    PUTSLAVE_CMD_ID_RST = 0x01,
    PUTSLAVE_CMD_ID_VER = 0x02,
    PUTSLAVE_CMD_ID_SER = 0x03,
    PUTSLAVE_CMD_ID_BREV = 0x04,
    PUTSLAVE_CMD_ID_BTIM = 0x05,
    PUTSLAVE_CMD_ID_MAC = 0x06,
    PUTSLAVE_CMD_ID_UID = 0x07,
    PUTSLAVE_CMD_ID_IP4 = 0x08,
    PUTSLAVE_CMD_ID_LOCK = 0x09,
    PUTSLAVE_CMD_ID_TST = 0x10,
    PUTSLAVE_CMD_ID_TONE = 0x11,
    PUTSLAVE_CMD_ID_START = 0x12,
    PUTSLAVE_CMD_ID_STOP = 0x13,
    PUTSLAVE_CMD_ID_TSTE = 0x14,
    PUTSLAVE_CMD_ID_EECL = 0x20,
    PUTSLAVE_CMD_ID_EEDEF = 0x21,
    PUTSLAVE_CMD_ID_FPWM = 0x22,
    PUTSLAVE_CMD_ID_FRPM = 0x23,
    PUTSLAVE_CMD_ID_FPSM = 0x24,
    PUTSLAVE_CMD_ID_FMEA = 0x25,
    PUTSLAVE_CMD_ID_GPCF = 0x26,
    PUTSLAVE_CMD_ID_DOER = 0x27,
    PUTSLAVE_CMD_ID_ADC = 0xc0,
    PUTSLAVE_CMD_ID_GPIO = 0xc1,
    PUTSLAVE_CMD_ID_GCODE = 0xc2,
    PUTSLAVE_CMD_ID_PWM = 0xc3,
    PUTSLAVE_CMD_ID_INVAL = 0xc4,
    PUTSLAVE_CMD_ID_DIAG = 0xc5,
    PUTSLAVE_CMD_ID_UART = 0xc6,
    PUTSLAVE_CMD_ID_I2C = 0xc7,
    PUTSLAVE_CMD_ID_TEN = 0xc8,
    PUTSLAVE_CMD_ID_MOVE = 0xc9,
    PUTSLAVE_CMD_ID_TDG = 0xd0,
    PUTSLAVE_CMD_ID_GPUP = 0xd1,
};

// static const uint32_t FLASH_START_ADRESS = 0x08020200;

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern void putslave_init(uartslave_t *pslave);

#ifdef __cplusplus
}
#endif //__cplusplus
