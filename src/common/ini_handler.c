// INI file handler (ini_handler.c)

#include "ini_handler.h"
#include "wui_api.h"
#include <string.h>
#include "ff.h"
#include "eeprom.h"
#include "strings.h"

#define MAX_UINT16 65535

static const char network_ini_file_name[] = "/lan_settings.ini"; //change -> change msgboxes in screen_lan_settings

static bool ini_string_match(const char *section, const char *section_var, const char *name, const char *name_var) {
    return strcmp(section_var, section) == 0 && strcmp(name_var, name) == 0;
}

static int ini_handler_func(void *user, const char *section, const char *name, const char *value) {

    ETH_config_t *tmp_config = (ETH_config_t *)user;

    if (ini_string_match(section, "lan_ip4", name, "type")) {
        if (strncasecmp(value, "DHCP", 4) == 0) {
            CHANGE_LAN_TO_DHCP(tmp_config->lan.flag);
            tmp_config->var_mask |= ETHVAR_MSK(ETHVAR_LAN_FLAGS);
        } else if (strncasecmp(value, "STATIC", 6) == 0) {
            CHANGE_LAN_TO_STATIC(tmp_config->lan.flag);
            tmp_config->var_mask |= ETHVAR_MSK(ETHVAR_LAN_FLAGS);
        }
    } else if (ini_string_match(section, "lan_ip4", name, "hostname")) {
        strlcpy(tmp_config->hostname, value, ETH_HOSTNAME_LEN + 1);
        tmp_config->var_mask |= ETHVAR_MSK(ETHVAR_HOSTNAME);
    } else if (ini_string_match(section, "lan_ip4", name, "address")) {
        if (ip4addr_aton(value, &tmp_config->lan.addr_ip4)) {
            tmp_config->var_mask |= ETHVAR_MSK(ETHVAR_LAN_ADDR_IP4);
        }
    } else if (ini_string_match(section, "lan_ip4", name, "mask")) {
        if (ip4addr_aton(value, &tmp_config->lan.msk_ip4)) {
            tmp_config->var_mask |= ETHVAR_MSK(ETHVAR_LAN_MSK_IP4);
        }
    } else if (ini_string_match(section, "lan_ip4", name, "gateway")) {
        if (ip4addr_aton(value, &tmp_config->lan.gw_ip4)) {
            tmp_config->var_mask |= ETHVAR_MSK(ETHVAR_LAN_GW_IP4);
        }
    } else {
        return 0; /* unknown section/name, error */
    }
    return 1;
}

uint8_t ini_save_file(const char *ini_save_str) {

    UINT ini_config_len = strlen(ini_save_str);
    UINT written_bytes = 0;
    FIL ini_file;

    f_unlink(network_ini_file_name);

    uint8_t i = f_open(&ini_file, network_ini_file_name, FA_WRITE | FA_CREATE_NEW);
    uint8_t w = f_write(&ini_file, ini_save_str, ini_config_len, &written_bytes);
    uint8_t c = f_close(&ini_file);

    if (i || w || c || written_bytes != ini_config_len)
        return 0;

    return 1;
}

uint8_t ini_load_file(void *user_struct) {
    UINT written_bytes = 0;
    FIL ini_file;
    ini_file_str_t ini_str;

    uint8_t file_init = f_open(&ini_file, network_ini_file_name, FA_READ);
    uint8_t file_read = f_read(&ini_file, &ini_str, MAX_INI_SIZE, &written_bytes);
    uint8_t file_close = f_close(&ini_file);

    if (file_init || file_read || file_close) {
        return 0;
    }

    if (ini_parse_string(ini_str, ini_handler_func, user_struct) < 0) {
        return 0;
    }
    return 1;
}
