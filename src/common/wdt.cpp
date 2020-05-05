// wdt.cpp - watchdog timers (IWDG, WWDG)

#include "wdt.h"
#include "config.h"
#include "stm32f4xx_hal.h"

#define WDT_IWDG_WARNING_DELAY 3000 // 3s warning delay (1s for some actions)
#define WDT_IWDG_RELOAD        4095 // 4s max period

#define WDT_WWDG_REFRESH_DELAY 32  // refresh every 32ms
#define WDT_WWDG_WINDOW        100 // ~22ms min period
#define WDT_WWDG_RELOAD        127 // ~48ms max period

extern "C" {

IWDG_HandleTypeDef hiwdg = { 0 }; // set Instance member to null
WWDG_HandleTypeDef hwwdg = { 0 }; // ..

extern void Error_Handler(void);

volatile unsigned int wdt_iwdg_counter = 0;
volatile unsigned char wdt_wwdg_counter = 0;

wdt_iwdg_warning_cb_t *wdt_iwdg_warning_cb = 0;

void wdt_iwdg_init(void) {
#ifdef WDT_IWDG_ENABLED
    hiwdg.Instance = IWDG;
    hiwdg.Init.Prescaler = IWDG_PRESCALER_32;
    hiwdg.Init.Reload = WDT_IWDG_RELOAD;
    if (HAL_IWDG_Init(&hiwdg) != HAL_OK) {
        Error_Handler();
    }
#endif //WDT_IWDG_ENABLED
}

void wdt_iwdg_refresh(void) {
#ifdef WDT_IWDG_ENABLED
    if (hiwdg.Instance) {
        HAL_IWDG_Refresh(&hiwdg);
        wdt_iwdg_counter = 0;
    }
#endif //WDT_IWDG_ENABLED
}

void wdt_wwdg_init(void) {
    hwwdg.Instance = WWDG;
    hwwdg.Init.Prescaler = WWDG_PRESCALER_8;
    hwwdg.Init.Window = WDT_WWDG_WINDOW;
    hwwdg.Init.Counter = WDT_WWDG_RELOAD;
    hwwdg.Init.EWIMode = WWDG_EWI_ENABLE;
}

void wdt_tick_1ms(void) {
#ifdef WDT_WWDG_ENABLED
    if (hwwdg.Instance) {
        if (wdt_wwdg_counter++ >= WDT_WWDG_REFRESH_DELAY) {
            if (__HAL_RCC_WWDG_IS_CLK_DISABLED()) {
                HAL_WWDG_Init(&hwwdg);
            } else {
                HAL_WWDG_Refresh(&hwwdg);
            }
            wdt_wwdg_counter = 0;
        }
    }
#endif //WDT_WWDG_ENABLED
#ifdef WDT_IWDG_ENABLED
    if (hiwdg.Instance) {
        if (wdt_iwdg_counter++ < WDT_IWDG_WARNING_DELAY)
            return;
        if (wdt_iwdg_warning_cb)
            wdt_iwdg_warning_cb();
    }
#endif //WDT_IWDG_ENABLED
}

void HAL_WWDG_EarlyWakeupCallback(WWDG_HandleTypeDef *hwwdg) {
    // TODO: handle this callback
}

void HAL_WWDG_MspInit(WWDG_HandleTypeDef *hwwdg) {
    if (hwwdg->Instance == WWDG) {
        // Peripheral clock enable
        __HAL_RCC_WWDG_CLK_ENABLE();
        // WWDG interrupt Init
        HAL_NVIC_SetPriority(WWDG_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(WWDG_IRQn);
    }
}

} //extern "C"

void watchdog_init() {
    wdt_iwdg_init();
    //wdt_wwdg_init();
}

void HAL_watchdog_refresh() {
    wdt_iwdg_refresh();
}
