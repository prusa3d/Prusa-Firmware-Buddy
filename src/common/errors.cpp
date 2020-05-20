#include "errors.h"
#include "str_utils.h"
#include <inttypes.h>

/// main error-table
static const err_t error_list[] = {
    { { // first item for 'not found'-usage
          ERR_CAT_UNDEF,
          ERR_SUBCAT_UNDEF,
          ERR_ITEM_UNDEF },
        "",
        "?????",
        "unknown error",
        ECODE_NONE },
    { { ERR_CAT_THERMAL,
          ERR_CAT_THERMAL_SUBCAT_HOTEND,
          ERR_CAT_THERMAL_SUBCAT_HOTEND_HFAIL },
        "",
        "Heating Failed E1",
        "Check the" QT_HSPACE "print head heater &" QT_HSPACE "thermistor wiring for" QT_HSPACE "possible damage.",
        ECODE_201 },
    { { ERR_CAT_THERMAL,
          ERR_CAT_THERMAL_SUBCAT_BED,
          ERR_CAT_THERMAL_SUBCAT_BED_HFAIL },
        "",
        "Heating Failed Bed",
        "Check the" QT_HSPACE "bed heater &" QT_HSPACE "thermistor wiring for" QT_HSPACE "possible damage.",
        ECODE_201 },
    { { ERR_CAT_THERMAL,
          ERR_CAT_THERMAL_SUBCAT_HOTEND,
          ERR_CAT_THERMAL_SUBCAT_HOTEND_RUNAWAY },
        "",
        "Therm. Runaway E1",
        "Check the" QT_HSPACE "print head thermistor wiring for" QT_HSPACE "possible damage.",
        ECODE_202 },
    { { ERR_CAT_THERMAL,
          ERR_CAT_THERMAL_SUBCAT_BED,
          ERR_CAT_THERMAL_SUBCAT_BED_RUNAWAY },
        "",
        "Therm. Runaway Bed",
        "Check the" QT_HSPACE "bed thermistor wiring for" QT_HSPACE "possible damage.",
        ECODE_202 },
    { { ERR_CAT_THERMAL,
          ERR_CAT_THERMAL_SUBCAT_HOTEND,
          ERR_CAT_THERMAL_SUBCAT_HOTEND_MAXTEMP },
        "",
        "MAXTEMP E1",
        "Check the" QT_HSPACE "print head thermistor wiring for" QT_HSPACE "possible damage.",
        ECODE_203 },
    { { ERR_CAT_THERMAL,
          ERR_CAT_THERMAL_SUBCAT_HOTEND,
          ERR_CAT_THERMAL_SUBCAT_HOTEND_MINTEMP },
        "",
        "MINTEMP E1",
        "Check the" QT_HSPACE "print head thermistor wiring for" QT_HSPACE "possible damage.",
        ECODE_204 },
    { { ERR_CAT_THERMAL,
          ERR_CAT_THERMAL_SUBCAT_BED,
          ERR_CAT_THERMAL_SUBCAT_BED_MAXTEMP },
        "",
        "MAXTEMP Bed",
        "Check the" QT_HSPACE "bed thermistor wiring for" QT_HSPACE "possible damage.",
        ECODE_203 },
    { { ERR_CAT_THERMAL,
          ERR_CAT_THERMAL_SUBCAT_BED,
          ERR_CAT_THERMAL_SUBCAT_BED_MINTEMP },
        "",
        "MINTEMP Bed",
        "Check the" QT_HSPACE "bed thermistor wiring for" QT_HSPACE "possible damage.",
        ECODE_204 },
    { { ERR_CAT_THERMAL,
          ERR_CAT_THERMAL_SUBCAT_INTERNAL,
          ERR_CAT_THERMAL_SUBCAT_INTERNAL_SCHEDULER },
        "",
        "Inactive Time Kill",
        "Internal scheduler error. Restart the" QT_HSPACE "printer.",
        ECODE_205 },
    { { ERR_CAT_SYSTEM,
          ERR_CAT_SYSTEM_SUBCAT_INTERNAL,
          ERR_CAT_SYSTEM_SUBCAT_INTERNAL_ESTOP },
        "",
        "M112 Shutdown",
        "Emergency stop M112, restart the" QT_HSPACE "printer.",
        ECODE_506 },
    { { ERR_CAT_SYSTEM,
          ERR_CAT_SYSTEM_SUBCAT_INTERNAL,
          ERR_CAT_SYSTEM_SUBCAT_INTERNAL_FW },
        "",
        "Firmware Error",
        "Firmware corrupted! Please flash the" QT_HSPACE "firmware again.",
        ECODE_508 },
    { { ERR_CAT_ELECTRO,
          ERR_CAT_ELECTRO_SUBCAT_MINDA,
          ERR_CAT_ELECTRO_SUBCAT_MINDA_WIRING },
        "",
        "Homing Error",
        "Check MINDA cable (connected and" QT_HSPACE "not damaged).",
        ECODE_309 },
    { { ERR_CAT_ELECTRO,
          ERR_CAT_ELECTRO_SUBCAT_MINDA,
          ERR_CAT_ELECTRO_SUBCAT_MINDA_MBL },
        "",
        "MBL Error",
        "Ensure the" QT_HSPACE "sheet is correctly placed and" QT_HSPACE "check MINDA sensor connection.",
        ECODE_310 },
};

static err_num_t actual_error;

#define ERR_ITEMS (sizeof(error_list) / sizeof(error_list[0]))

/// inner function (error-table item finding)
static const err_t *get_error_item(err_num_t err_item) {
    for (uint32_t i = 1; i < ERR_ITEMS; i++) // ie. skip first item in error-table
        if ((err_item.cat_num == error_list[i].err_num.cat_num) && (err_item.subcat_num == error_list[i].err_num.subcat_num) && (err_item.item_num == error_list[i].err_num.item_num))
            return (&error_list[i]);
    return (&error_list[0]);
}

/// set actual error
void set_actual_error(err_num_t err_num) {
    actual_error = err_num; // ~ set_actual_error(err_num.cat_num, err_num.subcat_num, err_num.item_num);
}

/// set actual error
void set_actual_error(err_category_t err_category, err_subcategory_t err_subcategory, err_item_t err_item) {
    actual_error.cat_num = err_category;
    actual_error.subcat_num = err_subcategory;
    actual_error.item_num = err_item;
}

/// error-table item finding
const err_t *get_actual_error(void) {
    return (get_error_item(actual_error));
}
