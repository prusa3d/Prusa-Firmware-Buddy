#include "errors.h"
#include "str_utils.hpp"
#include <inttypes.h>

static err_num_t actual_error;

#define ERR_ITEMS (sizeof(error_list) / sizeof(error_list[0]))

/// inner function (error-table item finding)
static const err_t *get_error_item(err_num_t err_item) {
    for (uint32_t i = 1; i < sizeof(error_list); ++i) // ie. skip first item in error-table
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
