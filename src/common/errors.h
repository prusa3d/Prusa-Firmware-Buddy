// errors.h
#pragma once

#include "errors_ext.h"

typedef struct {
    // 32 bit
    const char *err_title;
    const char *err_text;
    // 16 bit
    err_num_t err_num;
} err_t;

extern void set_actual_error(err_num_t err_item);
extern const err_t *get_actual_error(void);
