#pragma once
#include "timing.h"

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

typedef uint32_t (*time_fn_t)();

inline void delay__(time_fn_t time_fn, uint32_t delay) {
    const uint32_t time_to_wait = delay + 1; // fixes issue with maxint, now i have tu use <= instead <
    uint32_t start = time_fn();
    while ((time_fn() - start) <= time_to_wait)
        ;
}

inline void delay_ns(uint32_t ns) {
    delay__(ticks_ns, ns);
}

inline void delay_us(uint32_t us) {
    delay__(ticks_us, us);
}

inline void delay_ms(uint32_t ms) {
    delay__(ticks_ms, ms);
}

inline void delay_s(uint32_t s) {
    delay__(ticks_s, s);
}

#ifdef __cplusplus
}
#endif //__cplusplus
