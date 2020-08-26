// marlin_errors.h
#pragma once

#include <inttypes.h>

// Marlin errors
#define MARLIN_ERR_TMCDriverError 0x00 //
#define MARLIN_ERR_ProbingFailed  0x01 //
static const uint8_t MARLIN_ERR_MAX = MARLIN_ERR_ProbingFailed;

// error masks
#define MARLIN_ERR_MSK(e_id) ((uint64_t)1 << (e_id))
static const uint8_t MARLIN_ERR_MSK_ALL = MARLIN_ERR_MSK(MARLIN_ERR_TMCDriverError) | MARLIN_ERR_MSK(MARLIN_ERR_ProbingFailed);

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern const char *marlin_errors_get_name(uint8_t err_id);

#ifdef __cplusplus
}
#endif //__cplusplus
