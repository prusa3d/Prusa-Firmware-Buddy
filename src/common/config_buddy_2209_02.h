//config_buddy_2209_02.h - configuration file for 2209 variant (rev02)
#pragma once
#include <stdint.h>

#include <device/board.h>
#include "printers.h"
#include "MarlinPin.h"
#define PRUSA_MARLIN_API

//--------------------------------------
//DBG - debug/trace configuration
#define DBG_RTOS // use FreeRTOS (semaphore and osDelay instead of HAL_Delay)
#ifdef _DEBUG
    #define DBG_SWO // trace to swo port
    //#define DBG_UART     6 // trace to uart6 port
    //#define DBG_CDC     // trace to cdc port
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

//Swap SPI for LCD and TMC
//xBuddy 0.2.0 used SPI3 = LCD, SPI6 = TMC
//xBuddy 0.2.1 used SPI6 = LCD, SPI3 = TMC
//#define SWAP_LCD_TMC_SPI

//--------------------------------------
//Loadcell HX717 configuration
#if ((PRINTER_TYPE == PRINTER_PRUSA_MK404) || (PRINTER_TYPE == PRINTER_PRUSA_IXL))

    #define LOADCELL_HX717

#endif //((PRINTER_TYPE == PRINTER_PRUSA_MK404) || (PRINTER_TYPE == PRINTER_PRUSA_IXL))
#if (PRINTER_TYPE == PRINTER_PRUSA_XL)
    #if BOARD_IS_DWARF
        #define LOADCELL_HX717
    #endif

#endif

//--------------------------------------
//ADC Mux configuration
#if (BOARD_IS_XLBUDDY)
    #define ADC_EXT_MUX
#endif //(BOARD_TYPE == XBUDDY_BOARD)

//FANCTL - new software pwm fan control with rpm measurement and closed loop
#define NEW_FANCTL

//new pause settings
static const uint8_t PAUSE_NOZZLE_TIMEOUT = 45; // nozzle "sleep" after 45s inside paused state

#if BOARD_IS_DWARF
    // Marlin Interrupt priorities
    #define STEP_TIMER_IRQ_PRIO 2
    #define TEMP_TIMER_IRQ_PRIO 2
#else
    // Marlin Interrupt priorities
    #define STEP_TIMER_IRQ_PRIO 1
    #define TEMP_TIMER_IRQ_PRIO 2
#endif

#if BOARD_IS_XBUDDY || BOARD_IS_XLBUDDY
    // Power-panic interrupt priorities
    #define POWER_PANIC_IRQ_PRIO 1

    #if (POWER_PANIC_IRQ_PRIO != STEP_TIMER_IRQ_PRIO)
        #error "POWER_PANIC_IRQ should have the same preemption class as STEP_TIMER_IRQ"
    #endif
#endif

// ESP configs
#if (BOARD_IS_BUDDY)
    #define USE_ESP01_WITH_UART6
#endif

#if (BOARD_IS_XBUDDY)
    #define USE_ESP01_WITH_UART8
#endif

#if BOARD_IS_XLBUDDY
    #define USE_ESP01_WITH_UART8
#endif
