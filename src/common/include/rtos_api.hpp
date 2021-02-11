/**
 * @file bind_rtos.hpp
 * @author Radek Vana
 * @brief api header for RTOS, meant to be switched with other in unit tests
 * @date 2021-02-11
 */

#pragma once

#include <stdint.h>

namespace {
#include "FreeRTOS.h" //must apper before include task.h
#include "task.h"     //critical sections
#include "cmsis_os.h" //osDelay
};

class Rtos {
public:
    static inline void Delay(uint32_t millisec) { osDelay(millisec); }
    static inline uint32_t GetTick() { return HAL_GetTick(); }
};

class CriticalSection {
public:
    CriticalSection() { taskENTER_CRITICAL(); }
    ~CriticalSection() { taskEXIT_CRITICAL(); }
};
