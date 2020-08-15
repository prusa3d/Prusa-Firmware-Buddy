// bsod.c - blue screen of death
#include <algorithm>
#include <cmath>

#include "bsod.h"
#include "FreeRTOS.h"
#include "task.h"
#include "sound.hpp"
#include "wdt.h"

#ifndef HAS_GUI
    #error "HAS_GUI not defined"
#elif HAS_GUI

    #include <stdio.h>
    #include <stdarg.h>
    #include <stdlib.h>
    #include <string.h>
    #include <inttypes.h>

    #include "Rect16.h"
    #include "safe_state.h"
    #include "stm32f4xx_hal.h"
    #include "config.h"
    #include "gui.hpp"
    #include "term.h"
    #include "st7789v.hpp"
    #include "window_term.hpp"
    #include "jogwheel.h"
    #include "gpio.h"
    #include "sys.h"
    #include "hwio.h"
    #include "version.h"
    #include "window_qr.hpp"
    #include "support_utils.h"
    #include "str_utils.hpp"
    #include "guitypes.h"
    #include "../lang/i18n.h"

    /* FreeRTOS includes. */
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
    xMPU_SETTINGS xMPUSettings;         /*< The MPU settings are defined as part of the port layer.  THIS MUST BE THE SECOND MEMBER OF THE TCB STRUCT. */
    #endif

    ListItem_t xStateListItem;                /*< The list that the state list item of a task is reference from denotes the state of that task (Ready, Blocked, Suspended ). */
    ListItem_t xEventListItem;                /*< Used to reference a task from an event list. */
    UBaseType_t uxPriority;                   /*< The priority of the task.  0 is the lowest priority. */
    StackType_t *pxStack;                     /*< Points to the start of the stack. */
    char pcTaskName[configMAX_TASK_NAME_LEN]; /*< Descriptive name given to the task when created.  Facilitates debugging only. */
    /*lint !e971 Unqualified char types are allowed for strings and single characters only. */

    #if (portSTACK_GROWTH > 0)
    StackType_t *pxEndOfStack;     /*< Points to the end of the stack on architectures where the stack grows up from low memory. */
    #endif

    #if (portCRITICAL_NESTING_IN_TCB == 1)
    UBaseType_t uxCriticalNesting; /*< Holds the critical section nesting depth for ports that do not maintain their own count in the port layer. */
    #endif

    #if (configUSE_TRACE_FACILITY == 1)
    UBaseType_t uxTCBNumber;       /*< Stores a number that increments each time a TCB is created.  It allows debuggers to determine when a task has been deleted and then recreated. */
    UBaseType_t uxTaskNumber;      /*< Stores a number specifically for use by third party trace code. */
    #endif

    #if (configUSE_MUTEXES == 1)
    UBaseType_t uxBasePriority;    /*< The priority last assigned to the task - used by the priority inheritance mechanism. */
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

//current thread from FreeRTOS
extern PRIVILEGED_INITIALIZED_DATA TCB_t *volatile pxCurrentTCB;

constexpr uint8_t PADDING = 10;
    #define X_MAX (display::GetW() - PADDING * 2)

//! @brief Put HW into safe state, activate display safe mode and initialize it twice
static void stop_common(void) {
    hwio_safe_state();
    st7789v_enable_safe_mode();
    hwio_beeper_set_pwm(0, 0);
    display::Init();
    display::Init();
}

//! @brief print white error message on background
//!
//! It prints also firmware version on bottom of the screen.
//!
//! @param term input message
//! @param background_color background color
static void print_error(term_t *term, color_t background_color) {
    render_term(Rect16(10, 10, 220, 288), term, resource_font(IDR_FNT_NORMAL), background_color, COLOR_WHITE);
    display::DrawText(Rect16(10, 290, 220, 20), string_view_utf8::MakeCPUFLASH((const uint8_t *)project_version_full), resource_font(IDR_FNT_NORMAL), background_color, COLOR_WHITE);
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
    display::Clear(COLOR_RED_ALERT);
    term_t term;
    uint8_t buff[TERM_BUFF_SIZE(20, 16)];
    term_init(&term, 20, 16, buff);

    display::DrawText(Rect16(PADDING, PADDING, X_MAX, 22), string_view_utf8::MakeCPUFLASH((const uint8_t *)error), GuiDefaults::Font, //resource_font(IDR_FNT_NORMAL),
        COLOR_RED_ALERT, COLOR_WHITE);
    display::DrawLine(point_ui16(PADDING, 30), point_ui16(display::GetW() - 1 - PADDING, 30), COLOR_WHITE);

    term_printf(&term, module);
    term_printf(&term, "\n");

    render_term(Rect16(PADDING, 100, 220, 220), &term, GuiDefaults::Font, COLOR_RED_ALERT, COLOR_WHITE);

    static const char rp[] = "RESET PRINTER"; // intentionally not translated yet
    render_text_align(Rect16(PADDING, 260, X_MAX, 30), string_view_utf8::MakeCPUFLASH((const uint8_t *)rp), GuiDefaults::Font,
        COLOR_WHITE, COLOR_BLACK, { 0, 0, 0, 0 }, ALIGN_CENTER);

    jogwheel_init();
    gui_reset_jogwheel();

    //questionable placement - where now, in almost every BSOD timers are
    //stopped and Sound class cannot update itself for timing sound signals.
    //GUI is in the middle of refactoring and should be showned after restart
    //when timers and everything else is running again (info by - Robert/Radek)
    Sound_Play(eSOUND_TYPE_CriticalAlert);

    //cannot use jogwheel_signals  (disabled interrupt)
    while (1) {
        wdt_iwdg_refresh();
        if (!gpio_get(jogwheel_config.pinENC))
            sys_reset(); //button press
    }
}

void general_error_init() {
    __disable_irq();
    stop_common();

    jogwheel_init();
    gui_reset_jogwheel();

    //questionable placement - where now, in almost every BSOD timers are
    //stopped and Sound class cannot update itself for timing sound signals.
    //GUI is in the middle of refactoring and should be showned after restart
    //when timers and everything else is running again (info by - Rober/Radek)

    Sound_Play(eSOUND_TYPE_CriticalAlert);
}

void general_error_run() {
    //cannot use jogwheel_signals  (disabled interrupt)
    while (1) {
        wdt_iwdg_refresh();
        if (!gpio_get(jogwheel_config.pinENC))
            sys_reset(); //button press
    }
}

void temp_error(const char *error, const char *module, float t_noz, float tt_noz, float t_bed, float tt_bed) {
    char text[128];

    /// FIXME split heating, min/max temp and thermal runaway
    static const char bad_bed[] = "Check the heatbed heater & thermistor wiring for possible damage.";
    static const char bad_head[] = "Check the print head heater & thermistor wiring for possible damage.";

    if (module[0] != 'E') {
        snprintf(text, sizeof(text), bad_bed);
    } else {
        snprintf(text, sizeof(text), bad_head);
    }

    general_error_init();
    display::Clear(COLOR_RED_ALERT);

    // draw header
    display::DrawText(Rect16(13, 12, display::GetW() - 13, display::GetH() - 12), string_view_utf8::MakeCPUFLASH((const uint8_t *)error), GuiDefaults::Font, COLOR_RED_ALERT, COLOR_WHITE);

    // draw line
    display::DrawLine(point_ui16(10, 33), point_ui16(229, 33), COLOR_WHITE);

    // draw text (5 lines)
    term_t term;
    uint8_t buff[TERM_BUFF_SIZE(20, 16)];
    term_init(&term, 20, 16, buff);
    term_printf(&term, text);

    /// FIXME convert to DrawText & check drawing multiline text
    render_term(Rect16(PADDING, 31 + PADDING, X_MAX, 220), &term, GuiDefaults::Font, COLOR_RED_ALERT, COLOR_WHITE);

    /// draw "Scan me" text
    static const char *scan_me_text = "Scan me for details";
    display::DrawText(Rect16(52, 142, display::GetW() - 52, display::GetH() - 142), string_view_utf8::MakeCPUFLASH((const uint8_t *)scan_me_text), resource_font(IDR_FNT_SMALL), COLOR_RED_ALERT, COLOR_WHITE);

    /// draw arrow
    render_icon_align(Rect16(191, 147, 36, 81), IDR_PNG_arrow_scan_me, COLOR_RED_ALERT, 0);

    /// draw QR
    char qr_text[MAX_LEN_4QR + 1];
    /// FIXME Currently the only one error code working
    error_url_long(qr_text, sizeof(qr_text), 12201);
    constexpr uint8_t qr_size_px = 140;
    const Rect16 qr_rect = { 120 - qr_size_px / 2, 223 - qr_size_px / 2, qr_size_px, qr_size_px }; /// center = [120,223]
    window_qr_t win(nullptr, qr_rect);
    win.rect = qr_rect;
    window_qr_t *window = &win;
    win.text = qr_text;
    win.bg_color = COLOR_WHITE;

    //display::DrawLine(point_ui16(0, 175), point_ui16(display::GetW() - 1, 175), COLOR_WHITE);

    /// use PNG RAM for QR code image
    uint8_t *qrcode = (uint8_t *)0x10000000; //ccram
    uint8_t *qr_buff = qrcode + qrcodegen_BUFFER_LEN_FOR_VERSION(qr_version_max);

    if (generate_qr(qr_text, qrcode, qr_buff)) {
        draw_qr(qrcode, window);
    }

    /// draw short URL
    /// FIXME Currently the only one error code working
    error_url_short(qr_text, sizeof(qr_text), 12201);
    // this MakeRAM is safe - qr_text is a local buffer on stack
    render_text_align(Rect16(0, 293, display::GetW(), display::GetH() - 293), string_view_utf8::MakeRAM((const uint8_t *)qr_text), resource_font(IDR_FNT_SMALL), COLOR_RED_ALERT, COLOR_WHITE, padding_ui8(0, 0, 0, 0), ALIGN_HCENTER);
    //display::DrawText(Rect16(30, 293, display::GetW() - 30, display::GetH() - 293), qr_text, resource_font(IDR_FNT_SMALL), COLOR_RED_ALERT, COLOR_WHITE);

    while (1) {
        wdt_iwdg_refresh();
    }
}

void _bsod(const char *fmt, const char *file_name, int line_number, ...) {
    va_list args;
    va_start(args, line_number);
    __disable_irq(); //disable irq

    char tskName[configMAX_TASK_NAME_LEN];
    strlcpy(tskName, pxCurrentTCB->pcTaskName, sizeof(tskName));
    StackType_t *pTopOfStack = (StackType_t *)pxCurrentTCB->pxTopOfStack;
    StackType_t *pBotOfStack = pxCurrentTCB->pxStack;

    stop_common();

    #ifdef PSOD_BSOD
    display::Clear(COLOR_BLACK); //clear with black color
    //display::DrawIcon(point_ui16(75, 40), IDR_PNG_icon_pepa, COLOR_BLACK, 0);
    display::DrawIcon(point_ui16(75, 40), IDR_PNG_icon_pepa_psod, COLOR_BLACK, 0);
    display::DrawText(Rect16(25, 200, 200, 22), "Happy printing!", resource_font(IDR_FNT_BIG), COLOR_BLACK, COLOR_WHITE);
    #else
    display::Clear(COLOR_NAVY);           //clear with dark blue color
    term_t term;                          //terminal structure
    uint8_t buff[TERM_BUFF_SIZE(20, 16)]; //terminal buffer for 20x16
    term_init(&term, 20, 16, buff);       //initialize terminal structure (clear buffer etc)

    if (file_name != nullptr) {
        //remove text before "/" and "\", to get filename without path
        const char *pc;
        pc = strrchr(file_name, '/');
        if (pc != 0)
            file_name = pc + 1;
        pc = strrchr(file_name, '\\');
        if (pc != 0)
            file_name = pc + 1;
        {
            char text[TERM_PRINTF_MAX];

            int ret = vsnprintf(text, sizeof(text), fmt, args);

            const size_t range = ret < TERM_PRINTF_MAX ? ret : TERM_PRINTF_MAX;
            for (size_t i = 0; i < range; i++)
                term_write_char(&term, text[i]);
        }
        term_printf(&term, "\n");
        if (file_name != 0)
            term_printf(&term, "%s", file_name); //print filename
        if ((file_name != 0) && (line_number != -1))
            term_printf(&term, " "); //print space
        if (line_number != -1)
            term_printf(&term, "%d", line_number); //print line number
        if ((file_name != 0) || (line_number != -1))
            term_printf(&term, "\n"); //new line if there is filename or line number
    }

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
        wdt_iwdg_refresh();

        //TODO: safe delay with sleep
    }

    va_end(args);
}

    #ifdef configCHECK_FOR_STACK_OVERFLOW

static TaskHandle_t tsk_hndl = 0;
static signed char *tsk_name = 0;

extern "C" void vApplicationStackOverflowHook(TaskHandle_t xTask, signed char *pcTaskName) {
    tsk_hndl = xTask;
    tsk_name = pcTaskName;
    if (pcTaskName != nullptr && strlen((const char *)pcTaskName) > 20)
        _bsod("STACK OVERFLOW\nHANDLE %p\n%s", 0, 0, xTask, pcTaskName);
    else
        _bsod("STACK OVERFLOW\nHANDLE %p\nTaskname ERROR", 0, 0, xTask);
}

    #endif //configCHECK_FOR_STACK_OVERFLOW

    #ifndef PSOD_BSOD
//https://www.freertos.org/Debugging-Hard-Faults-On-Cortex-M-Microcontrollers.html

/*
+--------------------------------------------------------+-------------+-----------------+-------------+
|                       Fault type                       |   Handler   | Status Register |  Bit Name   |
+--------------------------------------------------------+-------------+-----------------+-------------+
| Bus error on a vector read error                       | HardFault   | HFSR            | VECTTBL     |
| Fault that is escalated to a hard fault                |             |                 | FORCED      |
| Fault on breakpoint escalation                         |             |                 | DEBUGEVT    |
| Fault on instruction access                            | MemManage   | MMFSR           | IACCVIOL    |
| Fault on direct data access                            |             |                 | DACCVIOL    |
| Context stacking, because of an MPU access violation   |             |                 | MSTKERR     |
| Context unstacking, because of an MPU access violation |             |                 | MUNSTKERR   |
| During lazy floating-point state preservation          |             |                 | MLSPERR     |
| During exception stacking                              | BusFault    | BFSR            | STKERR      |
| During exception unstacking                            |             |                 | UNSTKERR    |
| During instruction prefetching, precise                |             |                 | IBUSERR     |
| During lazy floating-point state preservation          |             |                 | LSPERR      |
| Precise data access error, precise                     |             |                 | PRECISERR   |
| Imprecise data access error, imprecise                 |             |                 | IMPRECISERR |
| Undefined instruction                                  | UsageFault  | UFSR            | UNDEFINSTR  |
| Attempt to enter an invalid instruction set state      |             |                 | INVSTATE    |
| Failed integrity check on exception return             |             |                 | INVPC       |
| Attempt to access a non-existing coprocessor           |             |                 | NOCPC       |
| Illegal unaligned load or store                        |             |                 | UNALIGNED   |
| Stack overflow                                         |             |                 | STKOF       |
| Divide By 0                                            |             |                 | DIVBYZERO   |
+--------------------------------------------------------+-------------+-----------------+-------------+
*/

void ScreenHardFault(void) {
        #define IACCVIOL_Msk  (1UL << 0)
        #define DACCVIOL_Msk  (1UL << 1)
        #define MSTKERR_Msk   (1UL << 4)
        #define MUNSTKERR_Msk (1UL << 3)
        #define MLSPERR_Msk   (1UL << 5)

        #define IACCVIOL_Txt  "Fault on instruction access"
        #define DACCVIOL_Txt  "Fault on direct data access"
        #define MSTKERR_Txt   "Context stacking, because of an MPU access violation"
        #define MUNSTKERR_Txt "Context unstacking, because of an MPU access violation"
        #define MLSPERR_Txt   "During lazy floating-point state preservation"

        #define MMARVALID_Msk (1UL << 7) // MemManage Fault Address Register (MMFAR) valid flag:

        #define STKERR_Msk      (1UL << 12)
        #define UNSTKERR_Msk    (1UL << 11)
        #define IBUSERR_Msk     (1UL << 8)
        #define LSPERR_Msk      (1UL << 13)
        #define PRECISERR_Msk   (1UL << 9)
        #define IMPRECISERR_Msk (1UL << 10)

        #define STKERR_Txt      "During exception stacking"
        #define UNSTKERR_Txt    "During exception unstacking"
        #define IBUSERR_Txt     "During instruction prefetching, precise"
        #define LSPERR_Txt      "During lazy floating-point state preservation "
        #define PRECISERR_Txt   "Precise data access error, precise"
        #define IMPRECISERR_Txt "Imprecise data access error, imprecise"

        #define BFARVALID_Msk (1UL << 15) // MemManage Fault Address Register (MMFAR) valid flag:

        #define UNDEFINSTR_Msk (1UL << 16)
        #define INVSTATE_Msk   (1UL << 17)
        #define INVPC_Msk      (1UL << 18)
        #define NOCPC_Msk      (1UL << 19)
        #define UNALIGNED_Msk  (1UL << 24)
        #define DIVBYZERO_Msk  (1UL << 25)

        #define UNDEFINSTR_Txt "Undefined instruction"
        #define INVSTATE_Txt   "Attempt to enter an invalid instruction set state "
        #define INVPC_Txt      "Failed integrity check on exception return  "
        #define NOCPC_Txt      "Attempt to access a non-existing coprocessor"
        #define UNALIGNED_Txt  "Illegal unaligned load or store"
        #define DIVBYZERO_Txt  "Divide By 0"
        //#define STKOF (1UL << 0)

        #define ROWS 21
        #define COLS 32

    __disable_irq(); //disable irq

    char tskName[configMAX_TASK_NAME_LEN];
    memset(tskName, '\0', sizeof(tskName) * sizeof(char)); // set to zeros to be on the safe side
    strlcpy(tskName, pxCurrentTCB->pcTaskName, sizeof(tskName));
    StackType_t *pTopOfStack = (StackType_t *)pxCurrentTCB->pxTopOfStack;
    StackType_t *pBotOfStack = pxCurrentTCB->pxStack;

    stop_common();

    display::Clear(COLOR_NAVY);               //clear with dark blue color
    term_t term;                              //terminal structure
    uint8_t buff[TERM_BUFF_SIZE(COLS, ROWS)]; //terminal buffer for 20x16
    term_init(&term, COLS, ROWS, buff);       //initialize terminal structure (clear buffer etc)

    term_printf(&term, "TASK: %s. ", tskName);

    switch ((SCB->CFSR) & (IACCVIOL_Msk | DACCVIOL_Msk | MSTKERR_Msk | MUNSTKERR_Msk | MLSPERR_Msk | STKERR_Msk | UNSTKERR_Msk | IBUSERR_Msk | LSPERR_Msk | PRECISERR_Msk | IMPRECISERR_Msk | UNDEFINSTR_Msk | INVSTATE_Msk | INVPC_Msk | NOCPC_Msk | UNALIGNED_Msk | DIVBYZERO_Msk)) {
    case IACCVIOL_Msk:
        term_printf(&term, IACCVIOL_Txt);
        break;
    case DACCVIOL_Msk:
        term_printf(&term, DACCVIOL_Txt);
        break;
    case MSTKERR_Msk:
        term_printf(&term, MSTKERR_Txt);
        break;
    case MUNSTKERR_Msk:
        term_printf(&term, MUNSTKERR_Txt);
        break;
    case MLSPERR_Msk:
        term_printf(&term, MLSPERR_Txt);
        break;

    case STKERR_Msk:
        term_printf(&term, STKERR_Txt);
        break;
    case UNSTKERR_Msk:
        term_printf(&term, UNSTKERR_Txt);
        break;
    case IBUSERR_Msk:
        term_printf(&term, IBUSERR_Txt);
        break;
    case LSPERR_Msk:
        term_printf(&term, LSPERR_Txt);
        break;
    case PRECISERR_Msk:
        term_printf(&term, PRECISERR_Txt);
        break;
    case IMPRECISERR_Msk:
        term_printf(&term, IMPRECISERR_Txt);
        break;

    case UNDEFINSTR_Msk:
        term_printf(&term, UNDEFINSTR_Txt);
        break;
    case INVSTATE_Msk:
        term_printf(&term, INVSTATE_Txt);
        break;
    case INVPC_Msk:
        term_printf(&term, INVPC_Txt);
        break;
    case NOCPC_Msk:
        term_printf(&term, NOCPC_Txt);
        break;
    case UNALIGNED_Msk:
        term_printf(&term, UNALIGNED_Txt);
        break;
    case DIVBYZERO_Msk:
        term_printf(&term, DIVBYZERO_Txt);
        break;

    default:
        term_printf(&term, "Multiple Errors CFSR :%08x", SCB->CFSR);
        break;
    }
    term_printf(&term, "\n");

    term_printf(&term, "bot: 0x%08x top: 0x%08x\n", pBotOfStack, pTopOfStack);

    //32 characters pre line
    term_printf(&term, "CPUID:%08x  ", SCB->CPUID);
    if (SCB->ICSR)
        term_printf(&term, "ICSR :%08x  ", SCB->ICSR);
    if (SCB->VTOR)
        term_printf(&term, "VTOR :%08x  ", SCB->VTOR);
    if (SCB->AIRCR)
        term_printf(&term, "AIRCR:%08x  ", SCB->AIRCR);
    if (SCB->SCR)
        term_printf(&term, "SCR  :%08x  ", SCB->SCR);
    if (SCB->CCR)
        term_printf(&term, "CCR  :%08x  ", SCB->CCR);
    if (SCB->SHCSR)
        term_printf(&term, "SHCSR:%08x  ", SCB->SHCSR);
    if (SCB->HFSR)
        term_printf(&term, "HFSR :%08x  ", SCB->HFSR);
    if (SCB->DFSR)
        term_printf(&term, "DFSR :%08x  ", SCB->DFSR);
    if ((SCB->CFSR) & MMARVALID_Msk)
        term_printf(&term, "MMFAR:%08x  ", SCB->MMFAR); //print this only if value is valid
    if ((SCB->CFSR) & BFARVALID_Msk)
        term_printf(&term, "BFAR :%08x  ", SCB->BFAR); //print this only if value is valid
    if (SCB->AFSR)
        term_printf(&term, "AFSR :%08x  ", SCB->AFSR);
    if (SCB->DFR)
        term_printf(&term, "DFR  :%08x  ", SCB->DFR);
    if (SCB->ADR)
        term_printf(&term, "ADR  :%08x  ", SCB->ADR);
    if (SCB->CPACR)
        term_printf(&term, "CPACR:%08x\n", SCB->CPACR);

    /*
    term_printf(&term, "r0 :%08x", r0);
    term_printf(&term, "r1 :%08x", r1);
    term_printf(&term, "r2 :%08x", r2);
    term_printf(&term, "r3 :%08x", r3);
    term_printf(&term, "r12:%08x", r12);
    term_printf(&term, "lr :%08x", lr);
    term_printf(&term, "pc :%08x", pc);
    term_printf(&term, "psr:%08x", psr);*/

    //const int addr_string_len = 10;//"0x12345678"
    const int strings_per_row = 3;
    int available_rows = term.rows - term.row - 1;
    //int available_chars = available_rows * COLS;
    int stack_sz = pTopOfStack - pBotOfStack;
    //int stack_chars_to_print = (addr_string_len +1)* stack_sz - stack_sz / 3;//+1 == space, - stack_sz / 3 .. 3rd string does not have a space
    int requested_rows = stack_sz / 3;

    StackType_t *lastAddr;
    if (requested_rows < available_rows)
        lastAddr = pBotOfStack - 1;
    else
        lastAddr = pTopOfStack - available_rows * strings_per_row;

    int space_counter = 0; //3rd string does not have a space behind it
    for (StackType_t *i = pTopOfStack; i != lastAddr; --i) {
        space_counter++;
        term_printf(&term, "0x%08x", *i);
        if (space_counter % 3)
            term_printf(&term, " ");
    }

    render_term(Rect16(10, 10, 220, 288), &term, resource_font(IDR_FNT_SMALL), COLOR_NAVY, COLOR_WHITE);
    display::DrawText(Rect16(10, 290, 220, 20), string_view_utf8::MakeCPUFLASH((const uint8_t *)project_version_full), resource_font(IDR_FNT_SMALL), COLOR_NAVY, COLOR_WHITE);

    while (1) //endless loop
    {
        wdt_iwdg_refresh();

        //TODO: safe delay with sleep
    }
}

    #endif //PSOD_BSOD

#else  //HAS_GUI
void _bsod(const char *fmt, const char *file_name, int line_number, ...) {}
void general_error(const char *error, const char *module) {}
void temp_error(const char *error, const char *module, float t_noz, float tt_noz, float t_bed, float tt_bed) {}
void ScreenHardFault(void) {}
void vApplicationStackOverflowHook(TaskHandle_t xTask, signed char *pcTaskName) {}
#endif //HAS_GUI
