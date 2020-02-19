// putslave.h
#ifndef _PUTSLAVE_H
#define _PUTSLAVE_H

#include "uartslave.h"

#define PUTSLAVE_CMD_ID_RST   0x01
#define PUTSLAVE_CMD_ID_VER   0x02
#define PUTSLAVE_CMD_ID_SER   0x03
#define PUTSLAVE_CMD_ID_BREV  0x04
#define PUTSLAVE_CMD_ID_BTIM  0x05
#define PUTSLAVE_CMD_ID_MAC   0x06
#define PUTSLAVE_CMD_ID_UID   0x07
#define PUTSLAVE_CMD_ID_IP4   0x08
#define PUTSLAVE_CMD_ID_LOCK  0x09
#define PUTSLAVE_CMD_ID_TST   0x10
#define PUTSLAVE_CMD_ID_TONE  0x11
#define PUTSLAVE_CMD_ID_START 0x12
#define PUTSLAVE_CMD_ID_STOP  0x13
#define PUTSLAVE_CMD_ID_TSTE  0x14
#define PUTSLAVE_CMD_ID_EECL  0x20
#define PUTSLAVE_CMD_ID_ADC   0xc0
#define PUTSLAVE_CMD_ID_GPIO  0xc1
#define PUTSLAVE_CMD_ID_GCODE 0xc2
#define PUTSLAVE_CMD_ID_PWM   0xc3
#define PUTSLAVE_CMD_ID_INVAL 0xc4
#define PUTSLAVE_CMD_ID_DIAG  0xc5
#define PUTSLAVE_CMD_ID_UART  0xc6
#define PUTSLAVE_CMD_ID_I2C   0xc7
#define PUTSLAVE_CMD_ID_TEN   0xc8
#define PUTSLAVE_CMD_ID_MOVE  0xc9
#define PUTSLAVE_CMD_ID_TDG   0xd0
#define PUTSLAVE_CMD_ID_GPUP  0xd1

#define FLASH_START_ADRESS 0x08020200

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern void putslave_init(uartslave_t *pslave);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // _PUTSLAVE_H
