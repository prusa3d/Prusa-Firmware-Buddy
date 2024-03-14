// marlin_errors.h
#pragma once

#include <inttypes.h>

// Marlin errors
enum {
    MARLIN_ERR_TMCDriverError, //
    MARLIN_ERR_ProbingFailed, //
    MARLIN_ERR_NozzleCleaningFailed,
    MARLIN_ERR_MAX = MARLIN_ERR_NozzleCleaningFailed,
};

// error masks
#define MARLIN_ERR_MSK(e_id) ((uint64_t)1 << (e_id))

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern const char *marlin_errors_get_name(uint8_t err_id);

#ifdef __cplusplus
}
#endif //__cplusplus
