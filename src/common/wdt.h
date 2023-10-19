// wdt.h - watchdog timers (IWDG, WWDG)
#pragma once
#include <device/hal.h>

typedef void(wdt_iwdg_warning_cb_t)(void); // IWDG warning callback prototype

#if defined(__cplusplus)
extern "C" {
#endif // defined(__cplusplus)

extern IWDG_HandleTypeDef hiwdg;
extern WWDG_HandleTypeDef hwwdg;

extern wdt_iwdg_warning_cb_t *wdt_iwdg_warning_cb; // IWDG warning callback

extern void wdt_iwdg_refresh(void); // IWDG refresh, called from C code

extern void wdt_wwdg_init(void);

extern void wdt_tick_1ms(void); // timer tick (IWDG warning), called from HAL_TIM_PeriodElapsedCallback/timer6

#if defined(__cplusplus)
} // extern "C"

void watchdog_init(); // initialization - called from marlin during setup (C++)

void HAL_watchdog_refresh(); // IWDG refresh - called from marlin thermal management (C++)

#endif // defined(__cplusplus)
