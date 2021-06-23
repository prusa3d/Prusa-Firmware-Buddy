// INI file handler (ini_handler.h)
#pragma once

#include <stdint.h>
#include "ini.h"

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

#define BUDDY_INI_LINE_SIZE 200 // maximum allowed chars in a single line
#define MAX_SECTION         50
#define MAX_NAME            50

typedef enum {
    BUDDY_INI_OK = 0,
    BUDDY_INI_ERROR,
    BUDDY_INI_FILE_READ
} buddy_ini_error;

buddy_ini_error ini_save_file(const char *ini_save_str);

buddy_ini_error ini_load_file(void *user_struct);

#ifdef __cplusplus
} // extern "C"
#endif //__cplusplus
