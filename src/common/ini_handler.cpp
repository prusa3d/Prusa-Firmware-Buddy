// INI file handler (ini_handler.c)

#include "ini_handler.h"
#include "settings_ini.hpp"
#include "wui_api.h"
#include <string.h>
#include "ff.h"
#include "string.h"

#define MAX_UINT16 65535

uint8_t ini_save_file(const char *ini_save_str) {
    uint8_t ret_val = 1; // returns 1 on success
    size_t s_written = 0;
    int err = -1;
    size_t str_len = strlen(ini_save_str);

    FILE *file = fopen(settings_ini::file_name, "w");

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

uint8_t ini_load_file(ini_handler handler, void *user_struct) {
    if (0 == ini_parse(settings_ini::file_name, handler, user_struct)) {
        return 1;
    } else {
        return 0;
    }
}
