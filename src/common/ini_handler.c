// INI file handler (ini_handler.c)

#include "ini_handler.h"
#include "wui_api.h"
#include "eeprom.h"
#include "string.h"

#define MAX_UINT16 65535

static const char ini_file_name[] = "/usb/prusa_printer_settings.ini"; //change -> change msgboxes in screen_lan_settings

static bool ini_string_match(const char *section, const char *section_var, const char *name, const char *name_var) {
    return strcmp(section_var, section) == 0 && strcmp(name_var, name) == 0;
}

static int ini_handler_func(void *user, const char *section, const char *name, const char *value) {

    ETH_config_t *tmp_config = (ETH_config_t *)user;

    if (ini_string_match(section, "eth::ipv4", name, "type")) {
        if (strncasecmp(value, "DHCP", 4) == 0) {
            CHANGE_FLAG_TO_DHCP(tmp_config->lan.flag);
            tmp_config->var_mask |= ETHVAR_MSK(ETHVAR_LAN_FLAGS);
        } else if (strncasecmp(value, "STATIC", 6) == 0) {
            CHANGE_FLAG_TO_STATIC(tmp_config->lan.flag);
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
    uint8_t ret_val = 1; // returns 1 on success
    size_t s_written = 0;
    int err = -1;
    size_t str_len = strlen(ini_save_str);

    FILE *file = fopen(ini_file_name, "w");

    if (NULL != file) {
        s_written = fwrite(ini_save_str, 1, str_len, file);
        err = fclose(file);
        // check for errors
        if ((s_written != str_len) || (0 != err)) {
            ret_val = 0;
        }
    } else {
        ret_val = 0;
    }

    return ret_val;
}

uint8_t ini_load_file(void *user_struct) {
    if (0 == ini_parse(ini_file_name, ini_handler_func, user_struct)) {
        return 1;
    } else {
        return 0;
    }
}
