
/*
 * FreeRTOS Kernel V10.5.1
 * Portion Copyright (C) 2017 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 * Portion Copyright (C) 2019 StMicroelectronics, Inc.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

#pragma once

/*-----------------------------------------------------------
 * Application specific definitions.
 *
 * These definitions should be adjusted for your particular hardware and
 * application requirements.
 *
 * These parameters and more are described within the 'configuration' section of the
 * FreeRTOS API documentation available on the FreeRTOS.org web site.
 *
 * See http://www.freertos.org/a00110.html
 *----------------------------------------------------------*/

/* Ensure definitions are only used by the compiler, and not by the assembler. */
#if defined(__ICCARM__) || defined(__ARMCC_VERSION) || defined(__GNUC__)
    #include <stdint.h>
extern uint32_t SystemCoreClock;
#endif

/* Clock configuration */
#define configUSE_16_BIT_TICKS 0
#define configCPU_CLOCK_HZ     (SystemCoreClock)
#define configTICK_RATE_HZ     ((TickType_t)1000)

#define configSUPPORT_DYNAMIC_ALLOCATION      0
#define configSUPPORT_STATIC_ALLOCATION       1
#define configMINIMAL_STACK_SIZE              32
#define configUSE_PREEMPTION                  1
#define configUSE_IDLE_HOOK                   0
#define configUSE_TICK_HOOK                   0
#define configUSE_MUTEXES                     1
#define configUSE_RECURSIVE_MUTEXES           1
#define configUSE_COUNTING_SEMAPHORES         1
#define configTASK_NOTIFICATION_ARRAY_ENTRIES 2
#define INCLUDE_vTaskDelay                    1
#define INCLUDE_vTaskSuspend                  1
#define configCHECK_FOR_STACK_OVERFLOW        2

/* The Cortex-M0+ port has no BASEPRI, so there is no syscall priority ceiling: the kernel
 * runs PendSV and SysTick at the lowest priority and the interrupt safe FreeRTOS API may be
 * called from any ISR. */
#define configMAX_PRIORITIES 7

void hal_panic();
#define configASSERT(x)           \
    if ((x) == 0) {               \
        taskDISABLE_INTERRUPTS(); \
        hal_panic();              \
    }

#define xPortPendSVHandler  PendSV_Handler
#define xPortSysTickHandler SysTick_Handler
#define vPortSVCHandler     SVC_Handler
