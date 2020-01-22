// bsod.c - blue screen of death
#include "bsod.h"
#include "stm32f4xx_hal.h"
#include "config.h"
#include "gui.h"
#include "term.h"
#include "st7789v.h"
#include "window_term.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "safe_state.h"
#include <inttypes.h>
#include <inttypes.h>
#include "jogwheel.h"
#include "gpio.h"
#include "sys.h"
#include "hwio.h"
#include "version.h"

/* FreeRTOS includes. */
#include "task.h"
#include "StackMacros.h"

//this is private struct definition from FreeRTOS
/*
 * Task control block.  A task control block (TCB) is allocated for each task,
 * and stores task state information, including a pointer to the task's context
 * (the task's run time environment, including register values)
 */
typedef struct tskTaskControlBlock {
    volatile StackType_t *pxTopOfStack; /*< Points to the location of the last item placed on the tasks stack.  THIS MUST BE THE FIRST MEMBER OF THE TCB STRUCT. */

#if (portUSING_MPU_WRAPPERS == 1)
    xMPU_SETTINGS xMPUSettings; /*< The MPU settings are defined as part of the port layer.  THIS MUST BE THE SECOND MEMBER OF THE TCB STRUCT. */
#endif

    ListItem_t xStateListItem; /*< The list that the state list item of a task is reference from denotes the state of that task (Ready, Blocked, Suspended ). */
    ListItem_t xEventListItem; /*< Used to reference a task from an event list. */
    UBaseType_t uxPriority; /*< The priority of the task.  0 is the lowest priority. */
    StackType_t *pxStack; /*< Points to the start of the stack. */
    char pcTaskName[configMAX_TASK_NAME_LEN]; /*< Descriptive name given to the task when created.  Facilitates debugging only. */ /*lint !e971 Unqualified char types are allowed for strings and single characters only. */

#if (portSTACK_GROWTH > 0)
    StackType_t *pxEndOfStack; /*< Points to the end of the stack on architectures where the stack grows up from low memory. */
#endif

#if (portCRITICAL_NESTING_IN_TCB == 1)
    UBaseType_t uxCriticalNesting; /*< Holds the critical section nesting depth for ports that do not maintain their own count in the port layer. */
#endif

#if (configUSE_TRACE_FACILITY == 1)
    UBaseType_t uxTCBNumber; /*< Stores a number that increments each time a TCB is created.  It allows debuggers to determine when a task has been deleted and then recreated. */
    UBaseType_t uxTaskNumber; /*< Stores a number specifically for use by third party trace code. */
#endif

#if (configUSE_MUTEXES == 1)
    UBaseType_t uxBasePriority; /*< The priority last assigned to the task - used by the priority inheritance mechanism. */
    UBaseType_t uxMutexesHeld;
#endif

#if (configUSE_APPLICATION_TASK_TAG == 1)
    TaskHookFunction_t pxTaskTag;
#endif

#if (configNUM_THREAD_LOCAL_STORAGE_POINTERS > 0)
    void *pvThreadLocalStoragePointers[configNUM_THREAD_LOCAL_STORAGE_POINTERS];
#endif

#if (configGENERATE_RUN_TIME_STATS == 1)
    uint32_t ulRunTimeCounter; /*< Stores the amount of time the task has spent in the Running state. */
#endif

#if (configUSE_NEWLIB_REENTRANT == 1)
    /* Allocate a Newlib reent structure that is specific to this task.
		Note Newlib support has been included by popular demand, but is not
		used by the FreeRTOS maintainers themselves.  FreeRTOS is not
		responsible for resulting newlib operation.  User must be familiar with
		newlib and must provide system-wide implementations of the necessary
		stubs. Be warned that (at the time of writing) the current newlib design
		implements a system-wide malloc() that must be provided with locks. */
    struct _reent xNewLib_reent;
#endif

#if (configUSE_TASK_NOTIFICATIONS == 1)
    volatile uint32_t ulNotifiedValue;
    volatile uint8_t ucNotifyState;
#endif

/* See the comments above the definition of
	tskSTATIC_AND_DYNAMIC_ALLOCATION_POSSIBLE. */
#if (tskSTATIC_AND_DYNAMIC_ALLOCATION_POSSIBLE != 0)
    uint8_t ucStaticallyAllocated; /*< Set to pdTRUE if the task is a statically allocated to ensure no attempt is made to free the memory. */
#endif

#if (INCLUDE_xTaskAbortDelay == 1)
    uint8_t ucDelayAborted;
#endif

} tskTCB;

/* The old tskTCB name is maintained above then typedefed to the new TCB_t name
below to enable the use of older kernel aware debuggers. */
typedef tskTCB TCB_t;
char FW_version_str[22] = { '\0' };

//current thread from FreeRTOS
extern PRIVILEGED_INITIALIZED_DATA TCB_t *volatile pxCurrentTCB;

#ifndef _DEBUG
extern IWDG_HandleTypeDef hiwdg; //watchdog handle
#endif //_DEBUG

#define PADDING 10
#define X_MAX (display->w - PADDING * 2)

//! @brief Put HW into safe state, activate display safe mode and initialize it twice
static void stop_common(void) {
    hwio_safe_state();
    st7789v_enable_safe_mode();
    hwio_beeper_set_pwm(0, 0);
    display->init();
    display->init();
}

//! @brief print white error message on background
//!
//! It prints also firmware version on bottom of the screen.
//!
//! @param term input message
//! @param background_color background color
static void print_error(term_t *term, color_t background_color) {
    render_term(rect_ui16(10, 10, 220, 288), term, gui_defaults.font, background_color, COLOR_WHITE);
    display->draw_text(rect_ui16(10, 290, 220, 20), project_version_full, gui_defaults.font, background_color, COLOR_WHITE);
}

//! @brief Marlin stopped
//!
//! Disable interrupts, print red error message and stop in infinite loop.
//!
//! Known possible reasons.
//! @n MSG_INVALID_EXTRUDER_NUM
//! @n MSG_T_THERMAL_RUNAWAY
//! @n MSG_T_HEATING_FAILED
//! @n MSG_T_MAXTEMP
//! @n MSG_T_MINTEMP
//! @n "Emergency stop (M112)"
//! @n "Inactive time kill"
//!
//! @param error Null terminated string shown in header
//! @param module Null terminated string shown in the rest of the screen
void general_error(const char *error, const char *module) {
    __disable_irq();
    stop_common();
    display->clear(COLOR_RED_ALERT);
    term_t term;
    uint8_t buff[TERM_BUFF_SIZE(20, 16)];
    term_init(&term, 20, 16, buff);

    display->draw_text(rect_ui16(PADDING, PADDING, X_MAX, 22), error, gui_defaults.font, //resource_font(IDR_FNT_NORMAL),
        COLOR_RED_ALERT, COLOR_WHITE);
    display->draw_line(point_ui16(PADDING, 30), point_ui16(display->w - PADDING, 30), COLOR_WHITE);

    term_printf(&term, module);
    term_printf(&term, "\n");

    render_term(rect_ui16(PADDING, 100, 220, 220), &term, gui_defaults.font, COLOR_RED_ALERT, COLOR_WHITE);

    render_text_align(rect_ui16(PADDING, 260, X_MAX, 30), "RESET PRINTER", gui_defaults.font,
        COLOR_WHITE, COLOR_BLACK, padding_ui8(0, 0, 0, 0), ALIGN_CENTER);

    jogwheel_init();
    gui_reset_jogwheel();

    //cannot use jogwheel_signals  (disabled interrupt)
    while (1) {
#ifndef _DEBUG
        HAL_IWDG_Refresh(&hiwdg);
#endif //_DEBUG
        if (!gpio_get(jogwheel_config.pinENC))
            sys_reset(); //button press
    }
}

void temp_error(const char *error, const char *module, float t_noz, float tt_noz, float t_bed, float tt_bed) {
    char buff[128];
    snprintf(buff, sizeof(buff),
        "The requested %s\ntemperature was not\nreached.\n\nNozzle temp: %d/%d\nBed temp: %d/%d",
        module, (int)t_noz, (int)tt_noz, (int)t_bed, (int)tt_bed);
    general_error(error, buff);
}

void _bsod(const char *fmt, const char *file_name, int line_number, ...) {
    va_list args;
    va_start(args, line_number);
    __disable_irq(); //disable irq

    char tskName[configMAX_TASK_NAME_LEN];
    strncpy(tskName, pxCurrentTCB->pcTaskName, configMAX_TASK_NAME_LEN);
    StackType_t *pTopOfStack = (StackType_t *)pxCurrentTCB->pxTopOfStack;
    StackType_t *pBotOfStack = pxCurrentTCB->pxStack;

    stop_common();

#ifdef PSOD_BSOD
    display->clear(COLOR_BLACK); //clear with black color
    //display->draw_icon(point_ui16(75, 40), IDR_PNG_icon_pepa, COLOR_BLACK, 0);
    display->draw_icon(point_ui16(75, 40), IDR_PNG_icon_pepa_psod, COLOR_BLACK, 0);
    display->draw_text(rect_ui16(25, 200, 200, 22), "Happy printing!", resource_font(IDR_FNT_BIG), COLOR_BLACK, COLOR_WHITE);
#else
    display->clear(COLOR_NAVY); //clear with dark blue color
    term_t term; //terminal structure
    uint8_t buff[TERM_BUFF_SIZE(20, 16)]; //terminal buffer for 20x16
    term_init(&term, 20, 16, buff); //initialize terminal structure (clear buffer etc)

    //remove text before "/" and "\", to get filename without path
    const char *pc;
    pc = strrchr(file_name, '/');
    if (pc != 0)
        file_name = pc + 1;
    pc = strrchr(file_name, '\\');
    if (pc != 0)
        file_name = pc + 1;

    vterm_printf(&term, fmt, args); //print text to terminal
    term_printf(&term, "\n");
    if (file_name != 0)
        term_printf(&term, "%s", file_name); //print filename
    if ((file_name != 0) && (line_number != -1))
        term_printf(&term, " "); //print space
    if (line_number != -1)
        term_printf(&term, "%d", line_number); //print line number
    if ((file_name != 0) || (line_number != -1))
        term_printf(&term, "\n"); //new line if there is filename or line number

    term_printf(&term, "TASK:%s\n", tskName);
    term_printf(&term, "b:%x", pBotOfStack);
    term_printf(&term, "t:%x", pTopOfStack);

    int lines_to_print = term.rows - term.row - 1;
    int stack_sz = pTopOfStack - pBotOfStack;

    StackType_t *lastAddr;
    if (stack_sz < lines_to_print * 2)
        lastAddr = pBotOfStack - 1;
    else
        lastAddr = pTopOfStack - 2 * lines_to_print;

    for (StackType_t *i = pTopOfStack; i != lastAddr; --i) {
        term_printf(&term, "%08x  ", *i);
    }

    print_error(&term, COLOR_NAVY);
#endif

    while (1) //endless loop
    {
#ifndef _DEBUG
        HAL_IWDG_Refresh(&hiwdg); //watchdog reset
#endif //_DEBUG

        //TODO: safe delay with sleep
    }

    va_end(args);
}
