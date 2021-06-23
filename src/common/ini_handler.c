// INI file handler (ini_handler.c)

#include "ini_handler.h"
#include "wui_api.h"
#include "ff.h"
#include "eeprom.h"
#include "strings.h"

#define MAX_UINT16 65535

static const char ini_file_name[] = "/usb/prusa_printer_settings.ini"; //change -> change msgboxes in screen_lan_settings

static bool ini_string_match(const char *section, const char *section_var, const char *name, const char *name_var) {
    return strcmp(section_var, section) == 0 && strcmp(name_var, name) == 0;
}

static int ini_handler_func(void *user, const char *section, const char *name, const char *value) {

    ETH_config_t *tmp_config = (ETH_config_t *)user;

    if (ini_string_match(section, "eth::ipv4", name, "type")) {
        if (strncmp(value, "DHCP", 4) == 0) {
            CHANGE_LAN_TO_DHCP(tmp_config->lan.flag);
            tmp_config->var_mask |= ETHVAR_MSK(ETHVAR_LAN_FLAGS);
        } else if (strncmp(value, "STATIC", 6) == 0) {
            CHANGE_LAN_TO_STATIC(tmp_config->lan.flag);
            tmp_config->var_mask |= ETHVAR_MSK(ETHVAR_LAN_FLAGS);
        }
    } else if (ini_string_match(section, "network", name, "hostname")) {
        strlcpy(tmp_config->hostname, value, ETH_HOSTNAME_LEN + 1);
        tmp_config->var_mask |= ETHVAR_MSK(ETHVAR_HOSTNAME);
    } else if (ini_string_match(section, "eth::ipv4", name, "addr")) {
        if (ip4addr_aton(value, &tmp_config->lan.addr_ip4)) {
            tmp_config->var_mask |= ETHVAR_MSK(ETHVAR_LAN_ADDR_IP4);
        }
    } else if (ini_string_match(section, "eth::ipv4", name, "mask")) {
        if (ip4addr_aton(value, &tmp_config->lan.msk_ip4)) {
            tmp_config->var_mask |= ETHVAR_MSK(ETHVAR_LAN_MSK_IP4);
        }
    } else if (ini_string_match(section, "eth::ipv4", name, "gw")) {
        if (ip4addr_aton(value, &tmp_config->lan.gw_ip4)) {
            tmp_config->var_mask |= ETHVAR_MSK(ETHVAR_LAN_GW_IP4);
        }
    } else if (ini_string_match(section, "network", name, "dns4")) {

        if (NULL != strchr(value, ';')) {
            char *token;
            char *rest = (char *)value;
            for (int i = 0; i < 2; i++) {
                token = strtok_r(rest, ";", &rest);
                if (NULL != token) {
                    switch (i) {
                    case 0:
                        if (ip4addr_aton(token, &tmp_config->dns1_ip4)) {
                            tmp_config->var_mask |= ETHVAR_MSK(ETHVAR_DNS1_IP4);
                        }
                        break;
                    case 1:
                        if (ip4addr_aton(token, &tmp_config->dns2_ip4)) {
                            tmp_config->var_mask |= ETHVAR_MSK(ETHVAR_DNS2_IP4);
                        }
                        break;
                    default:
                        break;
                    }
                }
            }
        } else {
            if (ip4addr_aton(value, &tmp_config->dns1_ip4)) {
                tmp_config->var_mask |= ETHVAR_MSK(ETHVAR_DNS1_IP4);
            }
        }
    }

    return 1;
}

uint8_t ini_save_file(const char *ini_save_str) {

    UINT ini_config_len = strlen(ini_save_str);
    UINT written_bytes = 0;
    FIL ini_file;

    f_unlink(ini_file_name);

    uint8_t i = f_open(&ini_file, ini_file_name, FA_WRITE | FA_CREATE_NEW);
    uint8_t w = f_write(&ini_file, ini_save_str, ini_config_len, &written_bytes);
    uint8_t c = f_close(&ini_file);

    if (i || w || c || written_bytes != ini_config_len)
        return 0;

    return 1;
}

uint8_t ini_load_file(void *user_struct) {
    return ini_parse(ini_file_name, ini_handler_func, user_struct);
}
