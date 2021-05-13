//dbg.c

#include <stdio.h>
#include "dbg.h"
#include <stdarg.h>
#include "stm32f4xx_hal.h"
#include "cmath_ext.h"

enum {
#ifndef _DEBUG
    DBG_MAXLINE = 128,
#else
    DBG_MAXLINE = 256,
#endif //_DEBUG
};

#ifdef DBG_RTOS

    #include "cmsis_os.h"
osSemaphoreId dbg_sema = 0; // semaphore handle

static inline void _dbg_lock(void) {
    if (dbg_sema == 0) {
        osSemaphoreDef(dbgSema);
        dbg_sema = osSemaphoreCreate(osSemaphore(dbgSema), 1);
    }
    osSemaphoreWait(dbg_sema, osWaitForever);
}

static inline void _dbg_unlock(void) {
    osSemaphoreRelease(dbg_sema);
}

    #define _dbg_delay osDelay

#else

    #define _dbg_lock()

    #define _dbg_unlock()

    #define _dbg_delay HAL_Delay

#endif //DBG_RTOS

#if defined(DBG_SWO)

void _dbg_swo(const char *fmt, ...) {
    _dbg_lock();

    char line[DBG_MAXLINE];
    va_list va;
    va_start(va, fmt);
    int len = vsnprintf(line, DBG_MAXLINE, fmt, va);
    va_end(va);

    if (len < 0) {
        _dbg_unlock();
        return;
    }
    len = MIN(len, DBG_MAXLINE - 1);

    for (int i = 0; i < len; ++i)
        ITM_SendChar(line[i]);
    ITM_SendChar('\n');

    _dbg_unlock();
}

#elif defined(DBG_UART)

    #if (DBG_UART == 1)
        #define _UART huart1
    #elif (DBG_UART == 2)
        #define _UART huart2
    #elif (DBG_UART == 3)
        #define _UART huart3
    #elif (DBG_UART == 4)
        #define _UART huart4
    #elif (DBG_UART == 5)
        #define _UART huart5
    #elif (DBG_UART == 6)
        #define _UART huart6
    #endif

extern UART_HandleTypeDef _UART;

void _dbg_uart(const char *fmt, ...) {
    _dbg_lock();

    char line[DBG_MAXLINE];
    va_list va;
    va_start(va, fmt);
    int len = vsnprintf(line, DBG_MAXLINE, fmt, va);
    va_end(va);

    if (len < 0) {
        _dbg_unlock();
        return;
    }
    len = MIN(len + 1, DBG_MAXLINE - 1);
    line[len - 1] = '\n';
    line[len] = '\0';

    HAL_UART_Transmit(&_UART, (uint8_t *)line, len, HAL_MAX_DELAY);

    _dbg_unlock();
}

#elif defined(DBG_CDC)

    #include "usbd_cdc_if.h"
static const uint8_t retries = 3;

void _dbg_cdc(const char *fmt, ...) {
    _dbg_lock();

    char line[DBG_MAXLINE];
    va_list va;
    va_start(va, fmt);
    int len = vsnprintf(line, DBG_MAXLINE, fmt, va);
    va_end(va);

    if (len < 0) {
        _dbg_unlock();
        return;
    }
    len = MIN(len + 1, DBG_MAXLINE - 1);
    line[len - 1] = '\n';
    line[len] = '\0';

    uint8_t retry = retries;
    while (retry--) {
        if (USBD_OK == CDC_Transmit_FS((uint8_t *)line, len))
            break;
        _dbg_delay(1);
    }

    _dbg_unlock();
}

#endif //

uint32_t _microseconds(void) {
    const int irq = __get_PRIMASK() & 1;
    if (irq)
        __disable_irq();
    const uint32_t u = TIM6->CNT;
    const uint32_t m = HAL_GetTick();
    if (irq)
        __enable_irq();
    return m * 1000 + u;
}
