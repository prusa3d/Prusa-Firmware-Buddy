// eeprom_loadsave.c

#include "eeprom_loadsave.h"
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "config.h"
#include "st25dv64k.h"
#include "dbg.h"
#include "cmsis_os.h"
#include "crc32.h"

// public functions for load/save

int eeprom_load_bin_from_usb(const char *fn) {
    int fd;
    uint8_t buff[128];
    uint16_t size = 0x800;
    uint16_t addr = 0x000;
    int bs = sizeof(buff);
    int br;
    fd = open(fn, O_RDONLY);
    if (fd >= 0) {
        while (size) {
            if (size < bs)
                bs = size;
            br = read(fd, buff, bs);
            if (br != bs)
                break;
            st25dv64k_user_write_bytes(addr, buff, bs);
            size -= bs;
            addr += bs;
        }
        close(fd);
        return (size == 0) ? 1 : 0;
    }
    return 0;
}

int eeprom_save_bin_to_usb(const char *fn) {
    int fd;
    uint8_t buff[128];
    uint16_t size = 0x800;
    uint16_t addr = 0x000;
    int bs = sizeof(buff);
    int bw;
    fd = open(fn, O_WRONLY | O_TRUNC);
    if (fd >= 0) {
        while (size) {
            if (size < bs)
                bs = size;
            st25dv64k_user_read_bytes(addr, buff, bs);
            bw = write(fd, buff, bs);
            if (bw != bs)
                break;
            size -= bs;
            addr += bs;
        }
        close(fd);
        return (size == 0) ? 1 : 0;
    }
    return 0;
}

int eeprom_load_xml_from_usb(const char *fn) {
    return 0;
}

int eeprom_save_xml_to_usb(const char *fn) {
    int fd;
    uint8_t id;
    char text[128];
    variant8_t var8;
    variant8_t *pvar = &var8;
    uint8_t var_count = eeprom_get_var_count();
    const char *var_name;
    fd = open(fn, O_WRONLY | O_TRUNC);
    if (fd >= 0) {
        write(fd, "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n", 55);
        write(fd, "<eeprom>\n", 9);
        for (id = 0; id < var_count; id++) {
            var8 = eeprom_get_var(id);
            *text = 0;
            eeprom_var_format(text, sizeof(text), id, var8);
            var_name = eeprom_get_var_name(id);
            write(fd, "  <variable id=\"", 16);
            write(fd, var_name, strlen(var_name));
            write(fd, "\" value=\"", 9);
            write(fd, text, strlen(text));
            write(fd, "\"/>\n", 4);
            variant8_done(&pvar);
        }
        write(fd, "</eeprom>\n", 10);
        close(fd);
        return 1;
    }
    return 0;
}
