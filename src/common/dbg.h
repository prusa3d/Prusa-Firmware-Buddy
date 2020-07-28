//dbg.h
#pragma once

#include <inttypes.h>
#include "config.h"

#ifndef DBG_LEVEL
    #define DBG_LEVEL 0
#endif //DBG_LEVEL

#if (DBG_LEVEL >= 3)
    #define _dbg3 _dbg
#else //(DBG_LEVEL >= 3)
    #define _dbg3(...)
#endif //(DBG_LEVEL >= 3)

#if (DBG_LEVEL >= 2)
    #define _dbg2 _dbg
#else //(DBG_LEVEL >= 2)
    #define _dbg2(...)
#endif //(DBG_LEVEL >= 2)

#if (DBG_LEVEL >= 1)
    #define _dbg1 _dbg
#else //(DBG_LEVEL >= 1)
    #define _dbg1(...)
#endif //(DBG_LEVEL >= 1)

#if (DBG_LEVEL >= 0)
    #define _dbg0 _dbg
#else //(DBG_LEVEL >= 0)
    #define _dbg0(...)
#endif //(DBG_LEVEL >= 0)

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

#if defined(DBG_SWO) // trace to SWO
extern void _dbg_swo(const char *fmt, ...);
    #define _dbg _dbg_swo
#elif defined(DBG_UART) // trace to UART
extern void _dbg_uart(const char *fmt, ...);
    #define _dbg _dbg_uart
#elif defined(DBG_CDC) // trace to CDC
extern void _dbg_cdc(const char *fmt, ...);
    #define _dbg _dbg_cdc
#else // trace disabled
    #define _dbg(...)
#endif //

extern uint32_t _microseconds(void);

#ifdef __cplusplus
}
#endif //__cplusplus
