/**
 * @file rtos_api.hpp
 * @author Radek Vana
 * @brief api header for RTOS, meant to be switched with other in unit tests
 * @date 2021-02-11
 */

#pragma once

#include "stm32f4xx_hal.h" //HAL_GetTick
#include "FreeRTOS.h" //must apper before include task.h
#include "task.h" //critical sections
#include "cmsis_os.h" //osDelay

class Rtos {
public:
    static inline void Delay(uint32_t millisec) { osDelay(millisec); }
    static inline uint32_t GetTick() { return HAL_GetTick(); }
};

/**
 * To be used for:
 * - Implementing atomic operations
 * on data shared between tasks and RTOS aware
 * interrupts.
 * - Precisely timed operations, which can
 * be interrupted for less than 10 microseconds
 * by high priority non OS aware interrupt.
 *
 * For atomic operations on stepper interrupt
 * data or timing with nanosecond precision use
 * DisableInterrupt instead.
 */
class CriticalSection {
public:
    CriticalSection() { taskENTER_CRITICAL(); }
    ~CriticalSection() { taskEXIT_CRITICAL(); }
};
