// eeprom_loadsave.c

#include "eeprom_loadsave.h"
#include <stdio.h>
#include <string.h>
#include "config.h"
#include "st25dv64k.h"
#include "dbg.h"
#include "cmsis_os.h"
#include "ff.h"
#include "crc32.h"

// public functions for load/save

int eeprom_load_bin_from_usb(const char *fn) {
    FIL fil;
    uint8_t buff[128];
    uint16_t size = 0x800;
    uint16_t addr = 0x000;
    UINT bs = sizeof(buff);
    UINT br;
    if (f_open(&fil, fn, FA_READ) == FR_OK) {
        while (size) {
            if (size < bs)
                bs = size;
            if (f_read(&fil, buff, bs, &br) != FR_OK)
                break;
            if (br != bs)
                break;
            st25dv64k_user_write_bytes(addr, buff, bs);
            size -= bs;
            addr += bs;
        }
        f_close(&fil);
        return (size == 0) ? 1 : 0;
    }
    return 0;
}

int eeprom_save_bin_to_usb(const char *fn) {
    FIL fil;
    uint8_t buff[128];
    uint16_t size = 0x800;
    uint16_t addr = 0x000;
    UINT bs = sizeof(buff);
    UINT bw;
    if (f_open(&fil, fn, FA_WRITE | FA_CREATE_ALWAYS) == FR_OK) {
        while (size) {
            if (size < bs)
                bs = size;
            st25dv64k_user_read_bytes(addr, buff, bs);
            if (f_write(&fil, buff, bs, &bw) != FR_OK)
                break;
            if (bw != bs)
                break;
            size -= bs;
            addr += bs;
        }
        f_close(&fil);
        return (size == 0) ? 1 : 0;
    }
    return 0;
}

int eeprom_load_xml_from_usb(const char *fn) {
    return 0;
}

int eeprom_save_xml_to_usb(const char *fn) {
    FIL fil;
    uint8_t id;
    char text[128];
    variant8_t var8;
    variant8_t *pvar = &var8;
    UINT bw;
    uint8_t var_count = eeprom_get_var_count();
    const char *var_name;
    if (f_open(&fil, fn, FA_WRITE | FA_CREATE_ALWAYS) == FR_OK) {
        f_write(&fil, "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n", 55, &bw);
        f_write(&fil, "<eeprom>\n", 9, &bw);
        for (id = 0; id < var_count; id++) {
            var8 = eeprom_get_var(id);
            *text = 0;
            eeprom_var_format(text, sizeof(text), id, var8);
            var_name = eeprom_get_var_name(id);
            f_write(&fil, "  <variable id=\"", 16, &bw);
            f_write(&fil, var_name, strlen(var_name), &bw);
            f_write(&fil, "\" value=\"", 9, &bw);
            f_write(&fil, text, strlen(text), &bw);
            f_write(&fil, "\"/>\n", 4, &bw);
            variant8_done(&pvar);
        }
        f_write(&fil, "</eeprom>\n", 10, &bw);
        f_close(&fil);
        return 1;
    }
    return 0;
}
