/**
 * @file rtos_api.hpp
 * @author Radek Vana
 * @brief api header for RTOS, meant to be switched with other in unit tests
 * @date 2021-02-11
 */

#pragma once

//warning include of <string.h> and "FreeRTOS.h"
//causing error: field 'xDummy17' has incomplete type '{anonymous}::_reent'
namespace {
#include "stm32f4xx_hal.h" //HAL_GetTick
#include "FreeRTOS.h"      //must apper before include task.h
#include "task.h"          //critical sections
#include "cmsis_os.h"      //osDelay
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
