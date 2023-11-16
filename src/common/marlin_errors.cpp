// marlin_errors.c

#include "marlin_errors.h"
#include <stdio.h>

// error name constants (dbg)
const char *__err_name[] = {
    "TMCDrivererror",
    "ProbingFailed",
};

// returns error name (dbg)
const char *marlin_errors_get_name(uint8_t err_id) {
    if (err_id <= MARLIN_ERR_MAX) {
        return __err_name[err_id];
    }
    return "";
}
