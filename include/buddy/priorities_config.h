#pragma once

#include <assert.h>
#include "device/board.h" // for BOARD_IS_xxx

#ifdef __cplusplus
extern "C" {
#endif
#include "FreeRTOSConfig.h" // for configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY
#include "SEGGER_RTT_Conf.h" // for SEGGER_RTT_MAX_INTERRUPT_PRIORITY
#include "cmsis_os.h" // for osPriorityXXX
#ifdef __cplusplus
}
#endif

#if BOARD_IS_BUDDY() || BOARD_IS_XBUDDY() || BOARD_IS_XLBUDDY()
/************************************************
 * Interrupt priorities:
 *************************************************/
    #define ISR_PRIORITY_TICK_TIMER 0
    #define ISR_PRIORITY_WWDG       0
    #if !BOARD_IS_XBUDDY()
        #define ISR_PRIORITY_ENDSTOP 0 // High priority to preempt the STEP_TIMER when possible
    #endif
    #define ISR_PRIORITY_PHASE_TIMER 1
    #define ISR_PRIORITY_STEP_TIMER  2
    #define ISR_PRIORITY_HX717_HARD  3
    #if BOARD_IS_XBUDDY()
        #define ISR_PRIORITY_ENDSTOP ISR_PRIORITY_HX717_HARD // Shared EXTI line: avoid STEP jitter
    #endif
    #define ISR_PRIORITY_TEMP_TIMER 4
// priority level defined by configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY - interrupts with lower priority below may use FreeRTOS calls
    #define ISR_PRIORITY_PUPPIES_USART 5 // needs to have priority higher than SEGGER_RTT_MAX_INTERRUPT_PRIORITY - which allows interruption of RTT lock
// priority level defined by SEGGER_RTT_MAX_INTERRUPT_PRIORITY - interrupts with lower priority may use RTT and SysView logging
    #define ISR_PRIORITY_POWER_PANIC   6
    #define ISR_PRIORITY_HX717_SOFT    6
    #define ISR_PRIORITY_ACCELEROMETER 6 // Accelerometer has to be able to preempt the move timer
    #define ISR_PRIORITY_MOVE_TIMER    7
    #define ISR_PRIORITY_DEFAULT       8 // default ISR priority, used by ISRs that don't need specific ISR priority
    #define ISR_PRIORITY_PENDSV        15

static_assert(configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY == 5);
static_assert(ISR_PRIORITY_PUPPIES_USART < SEGGER_RTT_MAX_INTERRUPT_PRIORITY);
static_assert(configLIBRARY_LOWEST_INTERRUPT_PRIORITY == 15);

    /************************************************
     * Task priorities
     *************************************************/
    #define TASK_PRIORITY_PUPPY_TASK          osPriorityRealtime
    #define TASK_PRIORITY_USB_DEVICE          osPriorityNormal
    #define TASK_PRIORITY_AC_FAULT            osPriorityRealtime
    #define TASK_PRIORITY_DEFAULT_TASK        osPriorityHigh
    #define TASK_PRIORITY_STARTUP             osPriorityHigh
    #define TASK_PRIORITY_METRIC_SYSTEM       osPriorityAboveNormal
    #define TASK_PRIORITY_USB_HOST            osPriorityNormal
    #define TASK_PRIORITY_USB_MSC_WORKER_HIGH osPriorityRealtime
    #define TASK_PRIORITY_USB_MSC_WORKER_LOW  osPriorityNormal
    #define TASK_PRIORITY_DISPLAY_TASK        osPriorityNormal
    #define TASK_PRIORITY_MEASUREMENT_TASK    osPriorityNormal
    #define TASK_PRIORITY_ESP_UPDATE          osPriorityNormal
    #define TASK_PRIORITY_LOG_TASK            osPriorityNormal
    #define TASK_PRIORITY_TCPIP_THREAD        osPriorityNormal
    #define TASK_PRIORITY_WUI                 osPriorityBelowNormal
    #define TASK_PRIORITY_CONNECT             osPriorityNormal
    #define TASK_PRIORITY_ASYNC_JOB_EXECUTOR  osPriorityBelowNormal

    // Media prefetch runs on async executor, but raises the priority temporarily when reading.
    // TASK_PRIORITY_CONNECT and TASK_PRIORITY_TCPIP_THREAD must be >= this value so that:
    // - The TCPIP stack can process incoming TCP segments and send ACKs while prefetch is running.
    // - The Connect task can write incoming transfer chunks to the download file while prefetch is
    //   reading it; starving Connect below prefetch causes a priority inversion where prefetch hits
    //   RESULT_OUT_OF_RANGE on data that Connect cannot deliver ("G-Code transfer running too slow").
    #define TASK_PRIORITY_MEDIA_PREFETCH osPriorityNormal

static_assert(configTIMER_TASK_PRIORITY == 5); // 5 is more than osPriorityRealtime

#elif BOARD_IS_DWARF()
/************************************************
 * Interrupt priorities:
 *************************************************/
static_assert(configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY == 0);
    #define IRQ_PRIORITY_DMA1_CHANNEL2_3 0
    #define ISR_PRIORITY_RS485           0
    #define ISR_PRIORITY_DMA1_CHANNEL1   0
    #define ISR_PRIORITY_TICK_TIMER      1
    #define ISR_PRIORITY_HX717           1
    #define ISR_PRIORITY_LIS2DH12        1
    #define ISR_PRIORITY_STEP_TIMER      2
    #define ISR_PRIORITY_TEMP_TIMER      2
    #define ISR_PRIORITY_MOVE_TIMER      2

    /************************************************
     * Task priorities
     *************************************************/
    #define TASK_PRIORITY_STARTUP        osPriorityHigh
    #define TASK_PRIORITY_MODBUS         osPriorityAboveNormal

#elif BOARD_IS_MODULARBED()
/************************************************
 * Interrupt priorities:
 *************************************************/
static_assert(configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY == 0);
    #define ISR_PRIORITY_PWM_TIMER       0
    #define IRQ_PRIORITY_DMA1_CHANNEL2_3 0
    #define ISR_PRIORITY_RS485           1
    #define ISR_PRIORITY_TICK_TIMER      3

    /************************************************
     * Task priorities
     *************************************************/
    #define TASK_PRIORITY_MODBUS         osPriorityAboveNormal
    #define TASK_PRIORITY_CONTROL        osPriorityNormal

#else
    #error Unknown board
#endif
