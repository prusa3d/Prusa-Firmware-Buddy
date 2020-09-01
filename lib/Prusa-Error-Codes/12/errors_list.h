// errors_list.h
#pragma once
#include "inttypes.h"
#include "../../src/lang/i18n.h"

static constexpr uint8_t ERR_PRINTER_CODE = 12;

typedef enum : uint16_t
{
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

typedef struct
{
    // 32 bit
    const char *err_title;
    const char *err_text;
    // 16 bit
    err_num_t err_num;
} err_t;

static constexpr err_t error_list[] = {
    {N_("Heating failed"),
     N_("Check the heatbed heater & thermistor wiring for possible damage."),
     ERR_TEMPERATURE_HEATING_BED},

    {N_("Heating failed"),
     N_("Check the print head heater & thermistor wiring for possible damage."),
     ERR_TEMPERATURE_HEATING_NOZZLE},

    {N_("Thermal Runaway"),
     N_("Check the heatbed thermistor wiring for possible damage."),
     ERR_TEMPERATURE_RUNAWAY_BED},

    {N_("Thermal Runaway"),
     N_("Check the print head thermistor wiring for possible damage."),
     ERR_TEMPERATURE_RUNAWAY_NOZZLE},

    {N_("MAXTEMP triggered"),
     N_("Check the heatbed thermistor wiring for possible damage."),
     ERR_TEMPERATURE_MAX_BED},

    {N_("MAXTEMP triggered"),
     N_("Check the print head thermistor wiring for possible damage."),
     ERR_TEMPERATURE_MAX_NOZZLE},

    {N_("MINTEMP triggered"),
     N_("Check the heatbed thermistor wiring for possible damage."),
     ERR_TEMPERATURE_MIN_BED},

    {N_("MINTEMP triggered"),
     N_("Check the print head thermistor wiring for possible damage."),
     ERR_TEMPERATURE_MIN_NOZZLE},
};
