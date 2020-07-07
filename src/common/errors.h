// errors.h
#pragma once

#include "inttypes.h"

typedef enum : int16_t {
    ERR_UNDEF = 0,

    ERR_MECHANICAL = 100,

    ERR_TEMPERATURE = 200,
    ERR_TEMPERATURE_HEATING_BED,
    ERR_TEMPERATURE_HEATING_NOZZLE,
    ERR_TEMPERATURE_RUNAWAY_BED,
    ERR_TEMPERATURE_RUNAWAY_NOZZLE,
    ERR_TEMPERATURE_MAX_BED,
    ERR_TEMPERATURE_MAX_NOZZLE,
    ERR_TEMPERATURE_MIN_BED,
    ERR_TEMPERATURE_MIN_NOZZLE,

    ERR_ELECTRO = 300,

    ERR_CONNECT = 400,

    ERR_SYSTEM = 500,

    ERR_OTHER = 900
} err_num_t;

typedef struct {
    // 32 bit
    const char *err_title;
    const char *err_text;
    // 16 bit
    err_num_t err_num;
} err_t;

extern void set_actual_error(err_num_t err_item);
extern const err_t *get_actual_error(void);
