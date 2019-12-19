// new_eeprom.cpp
#include "new_eeprom.h"
#include <inttypes.h>
#include <stddef.h>

//==============EEPROM REWORK=================

//static_assert(sizeof(EEPROM_t) <= EEPROM_MEMORY_RESERVED, "eeprom_t doesn't fit in it's reserved space in the memory.");
//EEPROM_t * const EEPROM = reinterpret_cast<EEPROM_t*> (EEPROM_START_ADDR);

void eeprom_initialize(EEPROM_t *ptr) { //fill up next condition if you add a variable to EEPROM
    static bool eeprom_init_flag = false;
    if (eeprom_init_flag == false) {
        if (ptr->eeprom_version < EEPROM_VER_OFFSET || ptr->eeprom_version > EEPROM_VER_OFFSET + EEPROM_VER) {
            eeprom_factory_reset(ptr);
            return;
        }
        if (ptr->eeprom_version == EEPROM_VER_OFFSET) {
            ptr->filament_type = 0;
            ptr->filament_r = 0;
            ptr->filament_g = 0;
            ptr->filament_b = 0;
        }
        /*-----------PREPARED_FOR_NEXT_VERSIONS-------------
		if(ptr->eeprom_version <= EEPROM_VER_OFFSET + 1)
		{

			//set variables that belong to eeprom version 2
		}
		if(ptr->eeprom_version <= EEPROM_VER_OFFSET + 2)
		{

			//set variables that belong to eeprom version 3
		}
	*/
        int8_t check_sum_flag = eeprom_check_sum(ptr); //sets eeprom's check sum

        if (ptr->eeprom_version == EEPROM_VER_OFFSET + EEPROM_VER) {
            if (check_sum_flag == 0) //data is corrupted
                eeprom_factory_reset(ptr);
        } else
            ptr->eeprom_version = EEPROM_VER_OFFSET + EEPROM_VER; //if you added some variable increment EEPROM_VER macro

        eeprom_init_flag = true;
    }
}

void eeprom_factory_reset(EEPROM_t *ptr) {

    ptr->eeprom_version = EEPROM_VER_OFFSET + EEPROM_VER;
    ptr->filament_type = 0;
    ptr->filament_r = 0;
    ptr->filament_g = 0;
    ptr->filament_b = 0;

    eeprom_check_sum(ptr);
}

int8_t eeprom_check_sum(EEPROM_t *ptr) {

    uint32_t curr_sum = ptr->check_sum;
    ptr->check_sum = 0; //checksum is calculated from whole EEPROM structure, where check_sum var = 0
    uint32_t tmp_sum = 0;
    const uint8_t *p = reinterpret_cast<const uint8_t *>(ptr);
    for (size_t count = sizeof(EEPROM_t); count; count--) {
        tmp_sum += *p++;
    }
    ptr->check_sum = tmp_sum;
    if (curr_sum != tmp_sum)
        return 0;
    else
        return 1;
}
//============================================
