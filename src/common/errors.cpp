#include <inttypes.h>

#include "errors.h"
#include "str_utils.hpp"

static ErrCode actual_error;

static const constexpr uint32_t ERR_ITEMS = sizeof(error_list) / sizeof(error_list[0]);

/// inner function (error-table item finding)
static const ErrDesc *get_error_item(ErrCode error_code) {
    for (uint32_t i = 0; i < sizeof(error_list); ++i) {
        if (error_code == error_list[i].err_code)
            return (&error_list[i]);
    }
    return nullptr;
}

/// set actual error
void set_actual_error(const ErrCode error_code) {
    actual_error = error_code;
}

/// error-table item finding
const ErrDesc *get_actual_error(void) {
    return get_error_item(actual_error);
}
