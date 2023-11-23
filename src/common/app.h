// app.h
#pragma once

#include <inttypes.h>

#ifdef __cplusplus

    #include "fanctl.hpp"

extern "C" {

#endif //__cplusplus

extern void app_setup(void);

extern void app_run(void);

extern void app_error(void);

extern void app_assert(uint8_t *file, uint32_t line);

extern void app_tim14_tick(void);

#ifdef __cplusplus
}
#endif //__cplusplus
