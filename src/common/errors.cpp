#include <inttypes.h>

#include "errors.h"
#include "str_utils.hpp"

static err_num_t actual_error;

static const constexpr uint32_t ERR_ITEMS = sizeof(error_list) / sizeof(error_list[0]);

/// inner function (error-table item finding)
static const err_t *get_error_item(err_num_t err_num) {
    for (uint32_t i = 0; i < sizeof(error_list); ++i) {
        if (err_num == error_list[i].err_num)
            return (&error_list[i]);
    }
    return nullptr;
}

/// set actual error
void set_actual_error(const err_num_t err_num) {
    actual_error = err_num;
}

/// error-table item finding
const err_t *get_actual_error(void) {
    return get_error_item(actual_error);
}
