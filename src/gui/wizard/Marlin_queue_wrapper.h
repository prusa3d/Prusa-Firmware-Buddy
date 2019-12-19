#ifndef _MARLIN_QUEUE_WRAPPER_
#define _MARLIN_QUEUE_WRAPPER_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

uint8_t get_gcode_queue_length(void);

#ifdef __cplusplus
} //extern "C"
#endif

#endif
