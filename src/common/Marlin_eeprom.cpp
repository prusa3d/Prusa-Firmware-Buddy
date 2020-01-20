// Marlin_eeprom.cpp

#include <stddef.h>
#include "stm32f4xx_hal.h"
#include "st25dv64k.h"
#include "dbg.h"

// this source implements four functions defined in Marlin/src/HAL/HAL_STM32F4/HAL.h:
//  eeprom_write_byte, eeprom_update_block, eeprom_read_byte, eeprom_read_block
// these functions are used in marlin PersistentStore api

#define DBG _dbg3 //debug level 3

#ifndef _DEBUG
extern "C" {
extern IWDG_HandleTypeDef hiwdg; //watchdog handle
}
#endif //_DEBUG

void eeprom_write_byte(uint8_t *pos, unsigned char value) {
    uint16_t adr = (uint16_t)(int)pos;
    st25dv64k_user_write(adr, value);
    DBG("EEwr %04x %02x", adr, value);
#ifndef _DEBUG
    HAL_IWDG_Refresh(&hiwdg); //watchdog reset
#endif //_DEBUG
}

uint8_t eeprom_read_byte(uint8_t *pos) {
    uint16_t adr = (uint16_t)(int)pos;
    uint8_t data;
    data = st25dv64k_user_read(adr);
    DBG("EErd %04x %02x", adr, data);
    return data;
}

void eeprom_read_block(void *__dst, const void *__src, size_t __n) {
    st25dv64k_user_read_bytes((uint16_t)(int)__src, __dst, __n);
}

void eeprom_update_block(const void *__src, void *__dst, size_t __n) {
    st25dv64k_user_write_bytes((uint16_t)(int)__dst, (void *)__src, __n);
#ifndef _DEBUG
    HAL_IWDG_Refresh(&hiwdg); //watchdog reset
#endif //_DEBUG
}
