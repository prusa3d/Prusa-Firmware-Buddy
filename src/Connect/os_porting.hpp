#pragma once

#ifdef __unix__
    #include <stdio.h>
    #include <stdlib.h>
    #include <unistd.h>
    #include <sys/time.h>
    #include <pthread.h>
    #include <semaphore.h>
    #include <stdint.h>
    #include <chrono>
    #include <thread>

uint32_t ticks_ms(void);
uint32_t osDelay(uint32_t millisec);
void osThreadTerminate(uint32_t);
    #define osThreadId uint32_t
osThreadId osThreadGetId();
size_t strlcpy(char *dst, const char *src, size_t maxlen);

#else
    #include <cmsis_os.h>
    #include "timing.h"
#endif
