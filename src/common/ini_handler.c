// INI file handler (ini_handler.c)

#include "ini_handler.h"
#include "wui_api.h"
#include "ff.h"
#include "eeprom.h"
#include "strings.h"

#define MAX_UINT16 65535
#define HANDLER    ini_handler_func

static const char ini_file_name[] = "/prusa_printer_settings.ini"; //change -> change msgboxes in screen_lan_settings

static bool ini_string_match(const char *section, const char *section_var, const char *name, const char *name_var) {
    return strcmp(section_var, section) == 0 && strcmp(name_var, name) == 0;
}

static int ini_handler_func(void *user, const char *section, const char *name, const char *value) {

    ETH_config_t *tmp_config = (ETH_config_t *)user;

    if (ini_string_match(section, "eth::ipv4", name, "type")) {
        if (strncmp(value, "DHCP", 4) == 0 || strncmp(value, "dhcp", 4) == 0) {
            CHANGE_LAN_TO_DHCP(tmp_config->lan.flag);
            tmp_config->var_mask |= ETHVAR_MSK(ETHVAR_LAN_FLAGS);
        } else if (strncmp(value, "STATIC", 6) == 0 || strncmp(value, "static", 6) == 0) {
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

        if (NULL != strchr(value, ':')) {
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

buddy_ini_error ini_save_file(const char *ini_save_str) {

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

buddy_ini_error ini_load_file(void *user) {
    FIL fileObject;
    buddy_ini_error error = BUDDY_INI_OK;
    char line[BUDDY_INI_LINE_SIZE];
    char *start;
    char *end;
    char *name;
    char *value;
    int lineno = 0;
    uint8_t skip_next_read = 0;

    char section[MAX_SECTION] = "";
    char prev_name[MAX_NAME] = "";

    // read line by line and parse it
    FRESULT res = f_open(&fileObject, ini_file_name, FA_READ);
    if (res) {
        f_close(&fileObject);
        return BUDDY_INI_FILE_READ;
    }

    /* Scan through stream line by line */
    while ((f_eof(&fileObject) == 0) && (f_error(&fileObject) == 0)) {
        if (NULL == f_gets(line, BUDDY_INI_LINE_SIZE, &fileObject)) {
            f_close(&fileObject);
            return BUDDY_INI_FILE_READ;
        }
        // skip the processing if it is part of previous line
        if (skip_next_read) {
            skip_next_read = 0;
            continue;
        }
        // check if the line was completely read
        if (NULL == strchr(line, '\n')) {
            skip_next_read = 1;
        }

#if INI_ALLOW_REALLOC && !INI_USE_STACK
        offset = strlen(line);
        while (offset == max_line - 1 && line[offset - 1] != '\n') {
            max_line *= 2;
            if (max_line > INI_MAX_LINE)
                max_line = INI_MAX_LINE;
            new_line = realloc(line, max_line);
            if (!new_line) {
                free(line);
                return -2;
            }
            line = new_line;
            if (reader(line + offset, (int)(max_line - offset), stream) == NULL)
                break;
            if (max_line >= INI_MAX_LINE)
                break;
            offset += strlen(line + offset);
        }
#endif

        lineno++;

        start = line;
#if INI_ALLOW_BOM
        if (lineno == 1 && (unsigned char)start[0] == 0xEF && (unsigned char)start[1] == 0xBB && (unsigned char)start[2] == 0xBF) {
            start += 3;
        }
#endif
        start = lskip(rstrip(start));

        if (strchr(INI_START_COMMENT_PREFIXES, *start)) {
            /* Start-of-line comment */
        }
#if INI_ALLOW_MULTILINE
        else if (*prev_name && *start && start > line) {
            /* Non-blank line with leading whitespace, treat as continuation
               of previous name's value (as per Python configparser). */
            if (!HANDLER(user, section, prev_name, start) && !error)
                error = lineno;
        }
#endif
        else if (*start == '[') {
            /* A "[section]" line */
            end = find_chars_or_comment(start + 1, "]");
            if (*end == ']') {
                *end = '\0';
                strncpy0(section, start + 1, sizeof(section));
                *prev_name = '\0';
#if INI_CALL_HANDLER_ON_NEW_SECTION
                if (!HANDLER(user, section, NULL, NULL) && !error)
                    error = lineno;
#endif
            } else if (!error) {
                /* No ']' found on section line */
                error = lineno;
            }
        } else if (*start) {
            /* Not a comment, must be a name[=:]value pair */
            end = find_chars_or_comment(start, "=:");
            if (*end == '=' || *end == ':') {
                *end = '\0';
                name = rstrip(start);
                value = end + 1;
#if INI_ALLOW_INLINE_COMMENTS
                end = find_chars_or_comment(value, NULL);
                if (*end)
                    *end = '\0';
#endif
                value = lskip(value);
                rstrip(value);

                /* Valid name[=:]value pair found, call handler */
                strncpy0(prev_name, name, sizeof(prev_name));
                if (!HANDLER(user, section, name, value) && !error)
                    error = lineno;
            } else if (!error) {
                /* No '=' or ':' found on name[=:]value line */
#if INI_ALLOW_NO_VALUE
                *end = '\0';
                name = rstrip(start);
                if (!HANDLER(user, section, name, NULL) && !error)
                    error = lineno;
#else
                error = lineno;
#endif
            }
        }

#if INI_STOP_ON_FIRST_ERROR
        if (error)
            break;
#endif
    }
    f_close(&fileObject);
    return error;
}
