//----------------------------------------------------------------------------//
// set safe state
#pragma once

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern void hwio_safe_state(void);

extern void hwio_low_power_state(void);

extern void buddy_disable_heaters(void);

#ifdef __cplusplus
}
#endif //__cplusplus
