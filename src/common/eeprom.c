// eeprom.c

#include "eeprom.h"
#include <stdio.h>
#include "st25dv64k.h"
#include "dbg.h"
#include <string.h>


#define EE_VAR_CNT (sizeof(eeprom_map) / sizeof(eeprom_entry_t))
#define EE_ADDRESS 0x0500


#pragma pack(push)
#pragma pack(1)

// eeprom map entry structure
typedef struct _eeprom_entry_t {
	uint8_t type;   // variant8 data type
	uint8_t count;  // number of elements
} eeprom_entry_t;

// eeprom vars structure (used for defaults)
typedef struct _eeprom_vars_t {
    uint16_t VERSION;
    uint8_t FILAMENT_TYPE;
    uint32_t FILAMENT_COLOR;
    float UNUSED_1;
    float UNUSED_2;
    float UNUSED_3;
    uint8_t RUN_SELFTEST;
    uint8_t RUN_XYZCALIB;
    uint8_t RUN_FIRSTLAY;
    uint8_t FSENSOR_ENABLED;
    float ZOFFSET;
    float PID_NOZ_P;
    float PID_NOZ_I;
    float PID_NOZ_D;
    float PID_BED_P;
    float PID_BED_I;
    float PID_BED_D;
    uint8_t LAN_FLAG;
    uint32_t LAN_IP4_ADDR;
	uint32_t LAN_IP4_MSK;
	uint32_t LAN_IP4_GW;
	uint32_t LAN_IP4_DNS1;
	uint32_t LAN_IP4_DNS2;
	char LAN_HOSTNAME[LAN_HOSTNAME_MAX_LEN + 1];
    char TEST[10];
} eeprom_vars_t;

#pragma pack(pop)


// eeprom map
const eeprom_entry_t eeprom_map[] = {
    { VARIANT8_UI16,  1  }, // EEVAR_VERSION
    { VARIANT8_UI8,   1  }, // EEVAR_FILAMENT_TYPE
    { VARIANT8_UI32,  1  }, // EEVAR_FILAMENT_COLOR
    { VARIANT8_FLT,   1  }, // EEVAR_UNUSED_1
    { VARIANT8_FLT,   1  }, // EEVAR_UNUSED_2
    { VARIANT8_FLT,   1  }, // EEVAR_UNUSED_3
    { VARIANT8_UI8,   1  }, // EEVAR_RUN_SELFTEST
    { VARIANT8_UI8,   1  }, // EEVAR_RUN_XYZCALIB
    { VARIANT8_UI8,   1  }, // EEVAR_RUN_FIRSTLAY
    { VARIANT8_UI8,   1  }, // EEVAR_FSENSOR_ENABLED
    { VARIANT8_FLT,   1  }, // EEVAR_ZOFFSET
    { VARIANT8_FLT,   1  }, // EEVAR_PID_NOZ_P
    { VARIANT8_FLT,   1  }, // EEVAR_PID_NOZ_I
    { VARIANT8_FLT,   1  }, // EEVAR_PID_NOZ_D
    { VARIANT8_FLT,   1  }, // EEVAR_PID_BED_P
    { VARIANT8_FLT,   1  }, // EEVAR_PID_BED_I
    { VARIANT8_FLT,   1  }, // EEVAR_PID_BED_D
    { VARIANT8_UI8,   1  }, // EEVAR_LAN_FLAG
    { VARIANT8_UI32,  1  }, // EEVAR_LAN_IP4_ADDR
    { VARIANT8_UI32,  1  }, // EEVAR_LAN_IP4_MSK
    { VARIANT8_UI32,  1  }, // EEVAR_LAN_IP4_GW
    { VARIANT8_UI32,  1  }, // EEVAR_LAN_IP4_DNS1
    { VARIANT8_UI32,  1  }, // EEVAR_LAN_IP4_DNS2
    { VARIANT8_PCHAR, LAN_HOSTNAME_MAX_LEN + 1 }, // EEVAR_LAN_HOSTNAME
    { VARIANT8_PCHAR, 10 }, // EEVAR_TEST
};

// eeprom variable names
const char *eeprom_var_name[] = {
    "VERSION",
    "FILAMENT_TYPE",
    "FILAMENT_COLOR",
    "UNUSED_1",
    "UNUSED_2",
    "UNUSED_3",
    "RUN_SELFTEST",
    "RUN_XYZCALIB",
    "RUN_FIRSTLAY",
    "FSENSOR_ENABLED",
	"ZOFFSET",
	"PID_NOZ_P",
	"PID_NOZ_I",
	"PID_NOZ_D",
	"PID_BED_P",
	"PID_BED_I",
	"PID_BED_D",
	"LAN_FLAG",
	"LAN_IP4_ADDR",
	"LAN_IP4_MSK",
	"LAN_IP4_GW",
	"LAN_IP4_DNS1",
	"LAN_IP4_DNS2",
	"LAN_HOSTNAME",
    "TEST",
};

// eeprom variable defaults
const eeprom_vars_t eeprom_var_defaults = {
	EEPROM_VERSION,  // EEVAR_VERSION
    0,               // EEVAR_FILAMENT_TYPE
    0,               // EEVAR_FILAMENT_COLOR
    0,               // EEVAR_UNUSED_1
    0,               // EEVAR_UNUSED_2
    0,               // EEVAR_UNUSED_3
    1,               // EEVAR_RUN_SELFTEST
    1,               // EEVAR_RUN_XYZCALIB
    1,               // EEVAR_RUN_FIRSTLAY
    0,               // EEVAR_FSENSOR_ENABLED
    0,               // EEVAR_ZOFFSET
    0,               // EEVAR_PID_NOZ_P
    0,               // EEVAR_PID_NOZ_I
    0,               // EEVAR_PID_NOZ_D
    0,               // EEVAR_PID_BED_P
    0,               // EEVAR_PID_BED_I
    0,               // EEVAR_PID_BED_D
	0,               // EEVAR_LAN_FLAG
	0,               // EEVAR_LAN_IP4_ADDR
	0,               // EEVAR_LAN_IP4_MSK
	0,               // EEVAR_LAN_IP4_GW
	0,               // EEVAR_LAN_IP4_DNS1
	0,               // EEVAR_LAN_IP4_DNS2
	"",              // EEVAR_LAN_HOSTNAME
	"TEST",          // EEVAR_TEST
};


//TODO: crc
uint16_t eeprom_crc_value = 0;
uint8_t eeprom_crc_index = 0;


// forward declarations of private functions

uint16_t eeprom_var_size(uint8_t id);
uint16_t eeprom_var_addr(uint8_t id);
void eeprom_dump(void);
void eeprom_print_vars(void);
void eeprom_clear(void);


// public functions

uint8_t eeprom_init(void) {
    uint8_t ret = 0;
    st25dv64k_init();
    //eeprom_clear();
    //eeprom_dump();
    uint16_t version = eeprom_get_var(EEVAR_VERSION).ui16;
    if (version != EEPROM_VERSION) {
        eeprom_defaults();
        ret = 1;
    }
    eeprom_print_vars();
    //eeprom_dump();
    return ret;
}

// write default values to all variables
void eeprom_defaults(void) {
    st25dv64k_user_write_bytes(EE_ADDRESS, (void*)&eeprom_var_defaults, sizeof(eeprom_vars_t));
}

variant8_t eeprom_get_var(uint8_t id) {
    uint16_t addr;
    uint16_t size;
    uint16_t data_size;
    void* data_ptr;
    variant8_t var = variant8_empty();
    if (id < EE_VAR_CNT) {
    	var = variant8_init(eeprom_map[id].type, eeprom_map[id].count, 0);
        size = eeprom_var_size(id);
        data_size = variant8_data_size(&var);
        if (size == data_size)
        {
            addr = eeprom_var_addr(id);
            data_ptr = variant8_data_ptr(&var);
            st25dv64k_user_read_bytes(addr, data_ptr, size);
        }
        else
        {
        	_dbg("error");
        	//TODO:error
        }
    }
    return var;
}

void eeprom_set_var(uint8_t id, variant8_t var) {
    uint16_t addr;
    uint16_t size;
    uint16_t data_size;
    void* data_ptr;
    if (id < EE_VAR_CNT) {
        if (var.type == eeprom_map[id].type) {
            size = eeprom_var_size(id);
            data_size = variant8_data_size(&var);
            if (size == data_size)
            {
                addr = eeprom_var_addr(id);
                data_ptr = variant8_data_ptr(&var);
                st25dv64k_user_write_bytes(addr, data_ptr, size);
            }
            else
            {
            	// TODO: error
            }
        }
        else
        {
        	// TODO: error
        }
    }
}


// private functions

uint16_t eeprom_var_size(uint8_t id) {
    if (id < EE_VAR_CNT)
    	return variant8_type_size(eeprom_map[id].type & ~VARIANT8_PTR) * eeprom_map[id].count;
    return 0;
}

uint16_t eeprom_var_addr(uint8_t id) {
    uint16_t addr = EE_ADDRESS;
    while (id)
        addr += eeprom_var_size(--id);
    return addr;
}

void eeprom_dump(void) {
    int i;
    int j;
    uint8_t b;
    char line[64];
    for (i = 0; i < 128; i++) // 128 lines = 2048 bytes
    {
        sprintf(line, "%04x", i * 16);
        for (j = 0; j < 16; j++) {
            b = st25dv64k_user_read(j + i * 16);
            sprintf(line + 4 + j * 3, " %02x", b);
        }
        _dbg("%s", line);
    }
}

int eeprom_var_sprintf(char *str, uint8_t id, variant8_t var) {
    switch (id) {
    case EEVAR_VERSION:
        return sprintf(str, "%u", (unsigned int)var.ui16);
    case EEVAR_FILAMENT_TYPE:
        return sprintf(str, "%u", (unsigned int)var.ui8);
    case EEVAR_FILAMENT_COLOR:
        return sprintf(str, "0x%08lx", (unsigned long)var.ui32);
    case EEVAR_UNUSED_1:
    case EEVAR_UNUSED_2:
    case EEVAR_UNUSED_3:
        return sprintf(str, "%.3f", (double)var.flt);
    case EEVAR_RUN_SELFTEST:
    case EEVAR_RUN_XYZCALIB:
    case EEVAR_RUN_FIRSTLAY:
    case EEVAR_FSENSOR_ENABLED:
        return sprintf(str, "%u", (unsigned int)var.ui8);
	case EEVAR_ZOFFSET:
	case EEVAR_PID_NOZ_P:
	case EEVAR_PID_NOZ_I:
	case EEVAR_PID_NOZ_D:
	case EEVAR_PID_BED_P:
	case EEVAR_PID_BED_I:
	case EEVAR_PID_BED_D:
        return sprintf(str, "%.3f", (double)var.flt);
    case EEVAR_TEST:
        return sprintf(str, "%10s", var.pch);
    }
    return 0;
}

void eeprom_print_vars(void) {
    uint8_t id;
    char text[128];
    variant8_t var8;
    for (id = 0; id < EE_VAR_CNT; id++) {
    	var8 = eeprom_get_var(id);
        eeprom_var_sprintf(text, id, var8);
        _dbg("%s=%s", eeprom_var_name[id], text);
        variant8_done(&var8);
    }
}

void eeprom_clear(void) {
    uint16_t a;
    uint32_t data = 0xffffffff;
    for (a = 0x0000; a < 0x0800; a += 4)
        st25dv64k_user_write_bytes(a, &data, 4);
}


int8_t eeprom_test_PUT(const unsigned int bytes) {
    unsigned int i;
    char line[16] = "abcdefghijklmno";
    char line2[16];
    uint8_t size = sizeof(line);
    unsigned int count = bytes / 16;

    for (i = 0; i < count; i++) {
        st25dv64k_user_write_bytes(EE_ADDRESS + i * size, &line, size);
    }

    int8_t res_flag = 1;

    for (i = 0; i < count; i++) {
        st25dv64k_user_read_bytes(EE_ADDRESS + i * size, &line2, size);
        if (strcmp(line2, line))
            res_flag = 0;
    }
    return res_flag;
}

#define ADDR_FILAMENT_TYPE 0x400

int eeprom_convert_from_v2(void)
{
//	eeprom_vars_t vars = eeprom_var_defaults;
//    st25dv64k_user_read_bytes(EE_ADDRESS + i * size, &(vars.FILAMENT_TYPE), size);
	return 0;
}

#include "ff.h"

int eeprom_load_bin(const char* fn)
{
    FIL fil;
    if (f_open(&fil, fn, FA_READ) == FR_OK) {

    }
	return 0;
}

int eeprom_save_bin(const char* fn)
{
    FIL fil;
    if (f_open(&fil, fn, FA_WRITE | FA_CREATE_ALWAYS) == FR_OK) {

    }
	return 0;
}

int eeprom_load_xml(const char* fn)
{
	return 0;
}

int eeprom_save_xml(const char* fn)
{
	return 0;
}
