//config_buddy_2209_02.h - configuration file for 2209 variant (rev02)
#pragma once
#include <stdint.h>

#define PRUSA_MARLIN_API

//--------------------------------------
//DBG - debug/trace configuration
#define DBG_RTOS // use FreeRTOS (semaphore and osDelay instead of HAL_Delay)
#ifdef _DEBUG
    #define DBG_SWO // trace to swo port
    //#define DBG_UART     6 // trace to uart6 port
    //#define DBG_CDC        // trace to cdc port
    #define DBG_LEVEL 1 // debug level (0..3)
#else
//#define DBG_SWO        // trace to swo port
#endif //_DEBUG

//--------------------------------------
//WDT - watchdog timers (IWDG, WWDG)
#ifndef _DEBUG
    #define WDT_IWDG_ENABLED
//#define WDT_WWDG_ENABLED
#endif //_DEBUG

//show filament sensor status in header
//#define DEBUG_FSENSOR_IN_HEADER

//--------------------------------------
//FANCTL - new software pwm fan control with rpm measurement and closed loop
#define NEW_FANCTL
#ifdef NEW_FANCTL

//FANCTLPRINT - printing fan
//static const uint8_t FANCTLPRINT_PWM_MIN = 15;
static const uint8_t FANCTLPRINT_PWM_MIN = 10;
static const uint8_t FANCTLPRINT_PWM_MAX = 50;
static const uint16_t FANCTLPRINT_RPM_MIN = 150;
static const uint16_t FANCTLPRINT_RPM_MAX = 5000;
static const uint8_t FANCTLPRINT_PWM_THR = 20;
//FANCTLHEATBREAK - heatbreak fan
//static const uint8_t FANCTLHEATBREAK_PWM_MIN = 12;
static const uint8_t FANCTLHEATBREAK_PWM_MIN = 0;
static const uint8_t FANCTLHEATBREAK_PWM_MAX = 50;
static const uint16_t FANCTLHEATBREAK_RPM_MIN = 1000;
static const uint16_t FANCTLHEATBREAK_RPM_MAX = 8000;
static const uint8_t FANCTLHEATBREAK_PWM_THR = 20;

#endif //NEW_FANCTL

//new pause settings
static const uint8_t PAUSE_NOZZLE_TIMEOUT = 45; // nozzle "sleep" after 45s inside paused state
// Marlin Interrupt priorities
#define STEP_TIMER_IRQ_PRIO 1
#define TEMP_TIMER_IRQ_PRIO 2
    // Power-panic interrupt priorities
#define POWER_PANIC_IRQ_PRIO 1

#if (POWER_PANIC_IRQ_PRIO != STEP_TIMER_IRQ_PRIO)
    #error "POWER_PANIC_IRQ should have the same preemption class as STEP_TIMER_IRQ"
#endif
#define USE_ESP01_WITH_UART6
