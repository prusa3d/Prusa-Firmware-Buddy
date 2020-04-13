#include "errors.h"
#include "str_utils.h"
#include <inttypes.h>

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
        "Heating failed E1",
        "Check the" QT_HSPACE "print head heater &" QT_HSPACE "thermistor wiring for" QT_HSPACE "possible damage.",
        ECODE_NONE },
    { { ERR_CAT_THERMAL,
          ERR_CAT_THERMAL_SUBCAT_BED,
          ERR_CAT_THERMAL_SUBCAT_BED_HFAIL },
        "",
        "Heating failed bed",
        "Check the" QT_HSPACE "bed heater &" QT_HSPACE "thermistor wiring for" QT_HSPACE "possible damage.",
        ECODE_NONE },
    { { ERR_CAT_THERMAL,
          ERR_CAT_THERMAL_SUBCAT_HOTEND,
          ERR_CAT_THERMAL_SUBCAT_HOTEND_RUNAWAY },
        "",
        "Therm. Runaway E1",
        "Check the" QT_HSPACE "print head thermistor wiring for" QT_HSPACE "possible damage.",
        ECODE_NONE },
    { { ERR_CAT_THERMAL,
          ERR_CAT_THERMAL_SUBCAT_BED,
          ERR_CAT_THERMAL_SUBCAT_BED_RUNAWAY },
        "",
        "Therm. Runaway bed",
        "Check the" QT_HSPACE "bed thermistor wiring for" QT_HSPACE "possible damage.",
        ECODE_NONE },
    { { ERR_CAT_THERMAL,
          ERR_CAT_THERMAL_SUBCAT_HOTEND,
          ERR_CAT_THERMAL_SUBCAT_HOTEND_MAXTEMP },
        "",
        "MAXTEMP E1",
        "Check the" QT_HSPACE "print head thermistor wiring for" QT_HSPACE "possible damage.",
        ECODE_NONE },
    { { ERR_CAT_THERMAL,
          ERR_CAT_THERMAL_SUBCAT_HOTEND,
          ERR_CAT_THERMAL_SUBCAT_HOTEND_MINTEMP },
        "",
        "MINTEMP E1",
        "Check the" QT_HSPACE "print head thermistor wiring for" QT_HSPACE "possible damage.",
        ECODE_NONE },
    { { ERR_CAT_THERMAL,
          ERR_CAT_THERMAL_SUBCAT_BED,
          ERR_CAT_THERMAL_SUBCAT_BED_MAXTEMP },
        "",
        "MAXTEMP bed",
        "Check the" QT_HSPACE "bed thermistor wiring for" QT_HSPACE "possible damage.",
        ECODE_NONE },
    { { ERR_CAT_THERMAL,
          ERR_CAT_THERMAL_SUBCAT_BED,
          ERR_CAT_THERMAL_SUBCAT_BED_MINTEMP },
        "",
        "MINTEMP bed",
        "Check the" QT_HSPACE "bed thermistor wiring for" QT_HSPACE "possible damage.",
        ECODE_NONE },
};

static err_num_t actual_error;

#define ERR_ITEMS (sizeof(error_list) / sizeof(error_list[0]))

static const err_t *get_error_item(err_num_t err_item) {
    for (uint32_t i = 1; i < ERR_ITEMS; i++) // ie. skip first item in error-table
        if ((err_item.cat_num == error_list[i].err_num.cat_num) && (err_item.subcat_num == error_list[i].err_num.subcat_num) && (err_item.item_num == error_list[i].err_num.item_num))
            return (&error_list[i]);
    return (&error_list[0]);
}

void set_actual_error(err_num_t err_num) {
    actual_error = err_num; // ~ set_actual_error(err_num.cat_num, err_num.subcat_num, err_num.item_num);
}

void set_actual_error(err_category_t err_category, err_subcategory_t err_subcategory, err_item_t err_item) {
    actual_error.cat_num = err_category;
    actual_error.subcat_num = err_subcategory;
    actual_error.item_num = err_item;
}

const err_t *get_actual_error(void) {
    return (get_error_item(actual_error));
}
