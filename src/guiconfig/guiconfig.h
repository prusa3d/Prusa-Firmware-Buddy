//guiconfig.h - guiapi configuration file
#pragma once

#include <inttypes.h>
#include <device/board.h>

///////////////////////////
// display type selector //
///////////////////////////
#if BOARD_IS_BUDDY
    #define USE_ST7789
    #define MENU_HAS_SCROLLBAR false
    #define MENU_HAS_BUTTONS   false
#else
    #error "macro BOARD_TYPE is not defined"
#endif

//--------------------------------------
//GUI configuration
#define GUI_USE_RTOS
#define GUI_JOGWHEEL_SUPPORT
#define GUI_WINDOW_SUPPORT

//--------------------------------------
//FreeRTOS Signals

//st7789v - spi DMA transmit complete (triggered from callback, gui thread is waiting for this signal)
static const uint32_t ST7789V_SIG_SPI_TX = 0x08;

static const uint32_t MENU_TIMEOUT_MS = 30000;
//--------------------------------------
//ST7789v configuration
#define ST7789V_USE_RTOS
#define ST7789V_PNG_SUPPORT
