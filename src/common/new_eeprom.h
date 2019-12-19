/*
 * Marlin_I2cEeprom.h
 *
 *  Created on: 7. 8. 2019
 *      Author: mcbig
 */

#ifndef NEW_EEPROM_H_
#define NEW_EEPROM_H_

#include <stdint.h>

//Filament
#define FILAMENT_ADDRESS 0x400
#define FILAMENT_COLOR_R 0x401
#define FILAMENT_COLOR_G 0x402
#define FILAMENT_COLOR_B 0x403

//firmware update flag
#define FW_UPDATE_FLAG_ADDRESS 0x40B

#define MENU_TIMEOUT_FLAG_ADDRESS 0x40C // is it ok?

//====================EEPROM_REWORK======================

#define EEPROM_START_ADDR 0x400
#define EEPROM_VER 0x01 //increment if you add any variable
#define EEPROM_VER_OFFSET 2323 //is added to EEPROM_VER for security
//#define EEPROM_MEMORY_RESERVED	0

typedef struct {

    uint16_t eeprom_version;
    uint32_t check_sum;
    uint8_t filament_type;
    uint8_t filament_r;
    uint8_t filament_g; //If you added a variable/s to the EEPROM make sure you increment EEPROM_VER macro
    uint8_t filament_b; //and add it/them to eeprom_initialize() and eeprom_factory_reset() accordingly
} EEPROM_t;

//extern EEPROM_t * const EEPROM;

#ifdef __cplusplus //Not included in sys.c (should be .cpp)

void eeprom_initialize(EEPROM_t *ptr); //set default values, makes eeprom compatible with it's older versions
extern void eeprom_factory_reset(EEPROM_t *ptr); //set all eerpom variables to default
int8_t eeprom_check_sum(EEPROM_t *ptr); //checks if data is not corrupted (optimalization at compilation will probably cause eeprom factory reset)

template <typename _T>
void eeprom_get(const _T &eeprom_source, _T &destination) {
    eeprom_initialize((EEPROM_t *)EEPROM_START_ADDR);
    const uint8_t *source_addr = reinterpret_cast<const uint8_t *>(&eeprom_source);
    uint8_t *ptr = reinterpret_cast<uint8_t *>(&destination);
    for (int count = sizeof(_T); count; --count, ++source_addr)
        *ptr++ = *source_addr;
}

template <typename _U>
void eeprom_put(_U &eeprom_destination, const _U &source) {
    eeprom_initialize((EEPROM_t *)EEPROM_START_ADDR);
    if (eeprom_destination != source) {
        uint8_t *dest_addr = reinterpret_cast<uint8_t *>(&eeprom_destination);
        const uint8_t *ptr = reinterpret_cast<const uint8_t *>(&source);
        for (int count = sizeof(_U); count; --count, ++dest_addr)
            *dest_addr = *ptr++;
        eeprom_check_sum((EEPROM_t *)EEPROM_START_ADDR);
    }
}
#endif //__cplusplus
//====================================================

#endif /* NEW_EEPROM_H_ */
