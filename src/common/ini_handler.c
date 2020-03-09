// INI file handler (ini_handler.c)

#include "ini_handler.h"
#include "ff.h"



static const char network_ini_file_name[] = "/lan_settings.ini"; //change -> change msgboxes in screen_lan_settings

uint8_t ini_save_file(const char * ini_save_str) {

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

uint8_t ini_load_file(ini_handler handler, void * user_struct) {
    UINT written_bytes = 0;
    FIL ini_file;

    uint8_t file_init = f_open(&ini_file, network_ini_file_name, FA_READ);
    uint8_t file_read = f_read(&ini_file, ini_file_str, MAX_INI_SIZE, &written_bytes);
    uint8_t file_close = f_close(&ini_file);

    if (file_init || file_read || file_close) {
        return 0;
    }

    if (ini_parse_string(ini_file_str, handler, user_struct) < 0) {
        return 0;
    }
    return 1;
}