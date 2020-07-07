// errors_ext.h
#pragma once
#include "inttypes.h"
#include "errors.h"

static constexpr uint8_t ERR_PRINTER_CODE = 12;

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

static constexpr err_t error_list[] = {
    { "Heating failed",
        "Check the heatbed heater & thermistor wiring for possible damage.",
        ERR_TEMPERATURE_HEATING_BED },

    { "Heating failed",
        "Check the print head heater & thermistor wiring for possible damage.",
        ERR_TEMPERATURE_HEATING_NOZZLE },

    { "Thermal Runaway",
        "Check the heatbed thermistor wiring for possible damage.",
        ERR_TEMPERATURE_RUNAWAY_BED },

    { "Thermal Runaway",
        "Check the print head thermistor wiring for possible damage.",
        ERR_TEMPERATURE_RUNAWAY_NOZZLE },

    { "MAXTEMP triggered",
        "Check the heatbed thermistor wiring for possible damage.",
        ERR_TEMPERATURE_MAX_BED },

    { "MAXTEMP triggered",
        "Check the print head thermistor wiring for possible damage.",
        ERR_TEMPERATURE_MAX_NOZZLE },

    { "MINTEMP triggered",
        "Check the heatbed thermistor wiring for possible damage.",
        ERR_TEMPERATURE_MIN_BED },

    { "MINTEMP triggered",
        "Check the print head thermistor wiring for possible damage.",
        ERR_TEMPERATURE_MIN_NOZZLE },
};
