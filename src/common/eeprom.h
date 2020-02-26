// eeprom.h

#ifndef _EEPROM_H
#define _EEPROM_H

#include "variant8.h"

#define LAN_HOSTNAME_MAX_LEN 20

#define EEVAR_VERSION            0x00
#define EEVAR_FILAMENT_TYPE      0x01
#define EEVAR_FILAMENT_COLOR     0x02
#define EEVAR_UNUSED_1           0x03
#define EEVAR_UNUSED_2           0x04
#define EEVAR_UNUSED_3           0x05
#define EEVAR_RUN_SELFTEST       0x06
#define EEVAR_RUN_XYZCALIB       0x07
#define EEVAR_RUN_FIRSTLAY       0x08
#define EEVAR_FSENSOR_ENABLED    0x09
#define EEVAR_LAN_FLAG           0x0A // lan_flag & 1 -> On = 0/off = 1, lan_flag & 2 -> dhcp = 0/static = 1
#define EEVAR_LAN_IP4_ADDR       0x0B // X.X.X.X address encoded in uint32
#define EEVAR_LAN_IP4_MSK        0x0C // X.X.X.X address encoded in uint32
#define EEVAR_LAN_IP4_GW         0x0D // X.X.X.X address encoded in uint32
#define EEVAR_LAN_IP4_DNS1       0x0E // X.X.X.X address encoded in uint32
#define EEVAR_LAN_IP4_DNS2       0x0F // X.X.X.X address encoded in uint32
#define EEVAR_LAN_HOSTNAME_START 0x10 //Start of 20char string
#define EEVAR_LAN_HOSTNAME_1     0x11
#define EEVAR_LAN_HOSTNAME_2     0x12
#define EEVAR_LAN_HOSTNAME_3     0x13
#define EEVAR_LAN_HOSTNAME_4     0x14
#define EEVAR_LAN_HOSTNAME_5     0x15
#define EEVAR_LAN_HOSTNAME_6     0x16
#define EEVAR_LAN_HOSTNAME_7     0x17
#define EEVAR_LAN_HOSTNAME_8     0x18
#define EEVAR_LAN_HOSTNAME_9     0x19
#define EEVAR_LAN_HOSTNAME_10    0x1A
#define EEVAR_LAN_HOSTNAME_11    0x1B
#define EEVAR_LAN_HOSTNAME_12    0x1C
#define EEVAR_LAN_HOSTNAME_13    0x1D
#define EEVAR_LAN_HOSTNAME_14    0x1E
#define EEVAR_LAN_HOSTNAME_15    0x1F
#define EEVAR_LAN_HOSTNAME_16    0x20
#define EEVAR_LAN_HOSTNAME_17    0x21
#define EEVAR_LAN_HOSTNAME_18    0x22
#define EEVAR_LAN_HOSTNAME_19    0x23 //End of 20char string

#define EEVAR_LAN_HOSTNAME_END (EEVAR_LAN_HOSTNAME_START + LAN_HOSTNAME_MAX_LEN) //End of 20char string

#define EEVAR_CONNECT_IP        0x24 // X.X.X.X address encoded in uint32
#define EEVAR_CONNECT_KEY_START 0x25 //Start of 20char string

#define LAN_EEFLG_ONOFF 1 //EEPROM flag for user-defined settings (SW turn OFF/ON of the LAN)
#define LAN_EEFLG_TYPE  2 //EEPROM flag for user-defined settings (Switch between dhcp and static)

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

// initialize eeprom
extern uint8_t eeprom_init(void);

// write default values to all variables
extern void eeprom_defaults(void);

// get variable value as variant8
extern variant8_t eeprom_get_var(uint8_t id);

// set variable value as variant8
extern void eeprom_set_var(uint8_t id, variant8_t var);

// fill range 0x0000..0x0800 with 0xff
extern void eeprom_clear(void);

extern void eeprom_get_string(uint8_t id, char *dest, uint16_t len);

extern void eeprom_set_string(uint8_t id, char *src, uint16_t len);

int8_t eeprom_test_PUT(const unsigned int);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_EEPROM_H
