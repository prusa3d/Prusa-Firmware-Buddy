// errors_list.h
#pragma once
#include "inttypes.h"
#include "i18n.h"

static constexpr uint8_t ERR_PRINTER_CODE = 12;

typedef enum : uint16_t {
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
    ERR_ELECTRO_HOMING_ERROR = 301,

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

static constexpr err_t error_list[] = {
    // r=1, c=19
    { N_("PREHEAT ERROR"),
        // r=5, c=20
        N_("Check the heatbed heater & thermistor wiring for possible damage."),
        ERR_TEMPERATURE_HEATING_BED },

    // r=1, c=19
    { N_("PREHEAT ERROR"),
        // r=5, c=20
        N_("Check the print head heater & thermistor wiring for possible damage."),
        ERR_TEMPERATURE_HEATING_NOZZLE },

    // r=1, c=19
    { N_("THERMAL RUNAWAY"),
        // r=5, c=20
        N_("Check the heatbed thermistor wiring for possible damage."),
        ERR_TEMPERATURE_RUNAWAY_BED },

    // r=1, c=19
    { N_("THERMAL RUNAWAY"),
        // r=5, c=20
        N_("Check the print head thermistor wiring for possible damage."),
        ERR_TEMPERATURE_RUNAWAY_NOZZLE },

    // r=1, c=19
    { N_("MAXTEMP ERROR"),
        // r=5, c=20
        N_("Check the heatbed thermistor wiring for possible damage."),
        ERR_TEMPERATURE_MAX_BED },

    // r=1, c=19
    { N_("MAXTEMP ERROR"),
        // r=5, c=20
        N_("Check the print head thermistor wiring for possible damage."),
        ERR_TEMPERATURE_MAX_NOZZLE },

    // r=1, c=19
    { N_("MINTEMP ERROR"),
        // r=5, c=20
        N_("Check the heatbed thermistor wiring for possible damage."),
        ERR_TEMPERATURE_MIN_BED },

    // r=1, c=19
    { N_("MINTEMP ERROR"),
        // r=5, c=20
        N_("Check the print head thermistor wiring for possible damage."),
        ERR_TEMPERATURE_MIN_NOZZLE },

    // r=1, c=19
    { N_("HOMING ERROR"),
        // r=5, c=20
        N_("SuperPINDA sensor is probably broken or disconnected, could not home Z-axis properly."),
        ERR_ELECTRO_HOMING_ERROR },
};
