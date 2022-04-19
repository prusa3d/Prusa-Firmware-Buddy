// bsod_gui.cpp - blue screen of death
#include "bsod.h"
#include "wdt.h"
#include "dump.h"
#include "safe_state.h"

#include "FreeRTOS.h"
#include "task.h"

#include <stdarg.h>

#include "sound.hpp"
#include "gui.hpp"
#include "Jogwheel.hpp"
#include "sys.h"
#include "hwio.h"
#include "version.h"
#include "support_utils.h"
#include "str_utils.hpp"
#include "../../lib/Prusa-Error-Codes/12_MINI/errors_list.h"
#include "../../lib/Marlin/Marlin/src/core/language.h"
#include "scratch_buffer.hpp"

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

    ListItem_t xStateListItem;                /*< The list that the state list item of a task is reference from denotes the state of that task (Ready, Blocked, Suspended ). */
    ListItem_t xEventListItem;                /*< Used to reference a task from an event list. */
    UBaseType_t uxPriority;                   /*< The priority of the task.  0 is the lowest priority. */
    StackType_t *pxStack;                     /*< Points to the start of the stack. */
    char pcTaskName[configMAX_TASK_NAME_LEN]; /*< Descriptive name given to the task when created.  Facilitates debugging only. */
    /*lint !e971 Unqualified char types are allowed for strings and single characters only. */

#if (portSTACK_GROWTH > 0)
    StackType_t *pxEndOfStack; /*< Points to the end of the stack on architectures where the stack grows up from low memory. */
#endif

#if (portCRITICAL_NESTING_IN_TCB == 1)
    UBaseType_t uxCriticalNesting; /*< Holds the critical section nesting depth for ports that do not maintain their own count in the port layer. */
#endif

#if (configUSE_TRACE_FACILITY == 1)
    UBaseType_t uxTCBNumber;  /*< Stores a number that increments each time a TCB is created.  It allows debuggers to determine when a task has been deleted and then recreated. */
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

//current thread from FreeRTOS
extern PRIVILEGED_INITIALIZED_DATA TCB_t *volatile pxCurrentTCB;

constexpr uint8_t PADDING = 10;
static const constexpr uint16_t X_MAX = display::GetW() - PADDING * 2;

//! @brief Put HW into safe state, activate display safe mode and initialize it twice
static void stop_common(void) {
    hwio_safe_state();
    st7789v_enable_safe_mode();
    hwio_beeper_set_pwm(0, 0);
    display::Init();
    display::Init();
}

void addFormatNum(char *buffer, const int size, int &position, const char *format, const uint32_t num) {
    int ret = snprintf(&buffer[position], std::max(0, size - position), format, num);
    if (ret > 0)
        position += ret;
    return;
}

void addFormatText(char *buffer, const int size, int &position, const char *format, const char *text) {
    int ret = snprintf(&buffer[position], std::max(0, size - position), format, text);
    if (ret > 0)
        position += ret;
    return;
}

void addText(char *buffer, const int size, int &position, const char *text) {
    addFormatText(buffer, size, position, "%s", text);
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
    const int COLS = 20;
    const int ROWS = 16;
    int buffer_size = COLS * ROWS + 1; ///< 7 bit ASCII allowed only (no UTF8)
    /// Buffer for text. PNG RAM cannot be used (font drawing).
    char buffer[buffer_size];
    int buffer_pos = 0; ///< position in buffer

    display::DrawText(Rect16(PADDING, PADDING, X_MAX, 22), string_view_utf8::MakeCPUFLASH((const uint8_t *)error), resource_font(IDR_FNT_NORMAL), //resource_font(IDR_FNT_NORMAL),
        COLOR_RED_ALERT, COLOR_WHITE);
    display::DrawLine(point_ui16(PADDING, 30), point_ui16(display::GetW() - 1 - PADDING, 30), COLOR_WHITE);

    addFormatText(buffer, buffer_size, buffer_pos, "%s\n", module);
    render_text_align(Rect16(PADDING, 60, 240, 260), string_view_utf8::MakeCPUFLASH((const uint8_t *)buffer), resource_font(IDR_FNT_NORMAL), COLOR_RED_ALERT, COLOR_WHITE, { 0, 0, 0, 0 }, { Align_t::LeftTop(), is_multiline::yes });

    static const char rp[] = "RESET PRINTER"; // intentionally not translated yet
    render_text_align(Rect16(PADDING, 260, X_MAX, 30), string_view_utf8::MakeCPUFLASH((const uint8_t *)rp), resource_font(IDR_FNT_NORMAL), COLOR_WHITE, COLOR_BLACK, { 0, 0, 0, 0 }, Align_t::Center());

    //questionable placement - where now, in almost every BSOD timers are
    //stopped and Sound class cannot update itself for timing sound signals.
    //GUI is in the middle of refactoring and should be showned after restart
    //when timers and everything else is running again (info by - Robert/Radek)
    Sound_Play(eSOUND_TYPE::CriticalAlert);

    //cannot use jogwheel_signals  (disabled interrupt)
    while (1) {
        wdt_iwdg_refresh();
        if (!jogwheel.GetJogwheelButtonPinState())
            sys_reset(); //button press
    }
}

void general_error_init() {
    __disable_irq();
    stop_common();

    //questionable placement - where now, in almost every BSOD timers are
    //stopped and Sound class cannot update itself for timing sound signals.
    //GUI is in the middle of refactoring and should be showned after restart
    //when timers and everything else is running again (info by - Rober/Radek)

    Sound_Play(eSOUND_TYPE::CriticalAlert);
}

void general_error_run() {
    //cannot use jogwheel_signals  (disabled interrupt)
    while (1) {
        wdt_iwdg_refresh();
        if (!jogwheel.GetJogwheelButtonPinState())
            sys_reset(); //button press
    }
}

//! Known possible reasons.
//! @n MSG_T_THERMAL_RUNAWAY
//! @n MSG_T_TEMPERATURE_SENSOR_STUCK
//! @n MSG_T_HEATING_FAILED
//! @n MSG_T_MAXTEMP
//! @n MSG_T_MINTEMP
//! @n "Emergency stop (M112)"
void draw_error_screen(const uint16_t error_code_short) {

    const uint16_t error_code = ERR_PRINTER_CODE * 1000 + error_code_short;

    /// search for proper text according to error code
    const char *text_title;
    const char *text_body;

    uint32_t i = 0;
    uint32_t count = sizeof(error_list) / sizeof(err_t);

    while (i < count && error_code_short != error_list[i].err_num) {
        ++i;
    }
    if (i == count) {
        /// no text found => leave blank screen
    } else {
        text_title = error_list[i].err_title;
        text_body = error_list[i].err_text;

        /// draw header & main text
        display::DrawText(Rect16(13, 12, display::GetW() - 13, display::GetH() - 12), _(text_title), resource_font(IDR_FNT_NORMAL), COLOR_RED_ALERT, COLOR_WHITE);
        display::DrawLine(point_ui16(10, 33), point_ui16(229, 33), COLOR_WHITE);
        render_text_align(Rect16(PADDING, 31 + PADDING, X_MAX, 220), _(text_body), resource_font(IDR_FNT_NORMAL), COLOR_RED_ALERT, COLOR_WHITE, { 0, 0, 0, 0 }, { Align_t::LeftTop(), is_multiline::yes });

        /// draw "Hand QR" icon
        render_icon_align(Rect16(20, 165, 64, 82), IDR_PNG_hand_qr, COLOR_RED_ALERT, Align_t::LeftTop());

        constexpr uint8_t qr_size_px = 140;
        const Rect16 qr_rect = { 160 - qr_size_px / 2, 200 - qr_size_px / 2, qr_size_px, qr_size_px }; /// center = [120,223]
        window_qr_t win(nullptr, qr_rect, error_code);
        win.Draw();

        char qr_text[window_qr_t::MAX_LEN_4QR + 1];
        /// draw short URL
        error_url_short(qr_text, sizeof(qr_text), error_code);
        // this MakeRAM is safe - qr_text is a local buffer on stack
        render_text_align(Rect16(0, 270, display::GetW(), display::GetH() - 255), string_view_utf8::MakeRAM((const uint8_t *)qr_text), resource_font(IDR_FNT_SMALL), COLOR_RED_ALERT, COLOR_WHITE, padding_ui8(0, 0, 0, 0), Align_t::CenterTop());

        /// draw footer information
        /// fw version, hash, [appendix], [fw signed]
        /// fw version
        char fw_version[13]; // intentionally limited to the number of practically printable characters without overwriting the nearby hash text
                             // snprintf will clamp the text if the input is too long
        snprintf(fw_version, sizeof(fw_version), "%s%s", project_version, project_version_suffix_short);
        render_text_align(Rect16(6, 295, 80, 10), string_view_utf8::MakeRAM((const uint8_t *)fw_version), resource_font(IDR_FNT_SMALL), COLOR_RED_ALERT, COLOR_WHITE, padding_ui8(0, 0, 0, 0), Align_t::CenterTop());
        /// hash
        if (eeprom_get_bool(EEVAR_DEVHASH_IN_QR)) {
            char p_code[PRINTER_CODE_SIZE + 1];
            printerCode(p_code);
            render_text_align(Rect16(98, 295, 64, 10), string_view_utf8::MakeRAM((const uint8_t *)p_code), resource_font(IDR_FNT_SMALL), COLOR_RED_ALERT, COLOR_WHITE, padding_ui8(0, 0, 0, 0), Align_t::CenterTop());
        }
        /// [appendix, fw signed]
        /// signed fw
        if (signature_exist()) {
            static const char signed_fw_str[] = "[S]";
            render_text_align(Rect16(160, 295, 40, 10), string_view_utf8::MakeCPUFLASH((const uint8_t *)signed_fw_str), resource_font(IDR_FNT_SMALL), COLOR_RED_ALERT, COLOR_WHITE, padding_ui8(0, 0, 0, 0), Align_t::CenterTop());
        }
        /// appendix
        if (appendix_exist()) {
            static const char appendix_str[] = "[A]";
            render_text_align(Rect16(185, 295, 40, 10), string_view_utf8::MakeCPUFLASH((const uint8_t *)appendix_str), resource_font(IDR_FNT_SMALL), COLOR_RED_ALERT, COLOR_WHITE, padding_ui8(0, 0, 0, 0), Align_t::CenterTop());
        }
    }
}

/// \returns nth character of the string
/// \returns \0 if the string is too short
char nth_char(const char str[], uint16_t nth) {
    while (nth > 0 && str[0] != 0) {
        --nth;
        ++str;
    }
    return str[0];
}

//! Known possible reasons.
//! @n MSG_T_THERMAL_RUNAWAY
//! @n MSG_T_HEATING_FAILED
//! @n MSG_T_MAXTEMP
//! @n MSG_T_MINTEMP
//! @n "Emergency stop (M112)"
void temp_error(const char *error, const char *module, float t_noz, float tt_noz, float t_bed, float tt_bed) {
    uint16_t *perror_code_short = (uint16_t *)(DUMP_INFO_ADDR + 1);
    //*((unsigned short *)(DUMP_INFO_ADDR + 1)) = code;

    /// Decision tree to define error code
    using namespace Language_en;
    if (module == nullptr) {
        /// TODO share these strings (saves ~100 B of binary size)
        if (strcmp(MSG_INVALID_EXTRUDER_NUM, error) == 0) {
            *perror_code_short = 0;
        } else if (strcmp("Emergency stop (M112)", error) == 0) {
            *perror_code_short = 510;
        } else if (strcmp("Inactive time kill", error) == 0) {
            *perror_code_short = 0;
        } else if (strcmp(MSG_ERR_HOMING, error) == 0) {
            *perror_code_short = 301;
        }
    } else {
        if (strcmp(MSG_HEATING_FAILED_LCD_BED, error) == 0) {
            *perror_code_short = 201;
        } else if (strcmp(MSG_HEATING_FAILED_LCD, error) == 0) {
            *perror_code_short = 202;
        } else if (strcmp(MSG_THERMAL_RUNAWAY_BED, error) == 0) {
            *perror_code_short = 203;
        } else if (strcmp(MSG_THERMAL_RUNAWAY, error) == 0) {
            *perror_code_short = 204;

        } else if (strcmp(MSG_ERR_MAXTEMP_BED, error) == 0) {
            *perror_code_short = 205;
        } else if (strcmp(MSG_ERR_MAXTEMP, error) == 0) {
            *perror_code_short = 206;
        } else if (strcmp(MSG_ERR_MINTEMP_BED, error) == 0) {
            *perror_code_short = 207;
        } else if (strcmp(MSG_ERR_MINTEMP, error) == 0) {
            *perror_code_short = 208;
        }
    }

    DUMP_TEMPERROR_TO_CCRAM();
    dump_to_xflash();
    sys_reset();
}

/// Draws error screen
/// Use for Debug only
void temp_error_code(const uint16_t error_code) {
    //    general_error_init();
    display::Clear(COLOR_RED_ALERT);
    draw_error_screen(error_code);
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
    //display::DrawIcon(point_ui16(75, 40), IDR_PNG_pepa_64px, COLOR_BLACK, 0);
    display::DrawIcon(point_ui16(75, 40), IDR_PNG_pepa_140px, COLOR_BLACK, 0);
    display::DrawText(Rect16(25, 200, 200, 22), "Happy printing!", resource_font(IDR_FNT_BIG), COLOR_BLACK, COLOR_WHITE);

#else

    display::Clear(COLOR_NAVY); ///< clear with dark blue color
    const int COLS = 32;
    const int ROWS = 21;
    int buffer_size = COLS * ROWS + 1; ///< 7 bit ASCII allowed only (no UTF8)
    /// Buffer for text. PNG RAM cannot be used (font drawing).
    char buffer[buffer_size];
    int buffer_pos = 0; ///< position in buffer
    buffer_pos += vsnprintf(&buffer[buffer_pos], std::max(0, buffer_size - buffer_pos), fmt, args);
    addText(buffer, buffer_size, buffer_pos, "\n");
    if (file_name != nullptr) {
        //remove text before "/" and "\", to get filename without path
        const char *pc;
        pc = strrchr(file_name, '/');
        if (pc != 0)
            file_name = pc + 1;
        pc = strrchr(file_name, '\\');
        if (pc != 0)
            file_name = pc + 1;
        if (file_name != nullptr)
            addFormatText(buffer, buffer_size, buffer_pos, "File: %s", file_name);
        if ((file_name != nullptr) && (line_number != -1))
            addText(buffer, buffer_size, buffer_pos, "\n");
        if (line_number != -1)
            addFormatNum(buffer, buffer_size, buffer_pos, "Line: %d", line_number);
        if ((file_name != nullptr) || (line_number != -1))
            addText(buffer, buffer_size, buffer_pos, "\n");
    }

    addFormatText(buffer, buffer_size, buffer_pos, "TASK:%s\n", tskName);
    addFormatNum(buffer, buffer_size, buffer_pos, "bot:0x%08x ", (uint32_t)pBotOfStack);
    addFormatNum(buffer, buffer_size, buffer_pos, "top:0x%08x\n", (uint32_t)pTopOfStack);

    const int lines = str2multiline(buffer, buffer_size, COLS);
    const int lines_to_print = ROWS - lines - 1;
    int stack_sz = pTopOfStack - pBotOfStack;

    StackType_t *lastAddr;
    if (stack_sz < lines_to_print * 2)
        lastAddr = pBotOfStack - 1;
    else
        lastAddr = pTopOfStack - 2 * lines_to_print;

    for (StackType_t *i = pTopOfStack; i != lastAddr; --i) {
        addFormatNum(buffer, buffer_size, buffer_pos, "0x%08x ", (uint32_t)*i);
    }
    render_text_align(Rect16(8, 10, 230, 290), string_view_utf8::MakeCPUFLASH((const uint8_t *)buffer), resource_font(IDR_FNT_SMALL), COLOR_NAVY, COLOR_WHITE, { 0, 0, 0, 0 }, { Align_t::LeftTop(), is_multiline::yes });
    display::DrawText(Rect16(8, 290, 220, 20), string_view_utf8::MakeCPUFLASH((const uint8_t *)project_version_full), resource_font(IDR_FNT_NORMAL), COLOR_NAVY, COLOR_WHITE);

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
    if (pcTaskName != nullptr && strlen((const char *)pcTaskName) < 20)
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
    static const constexpr uint32_t IACCVIOL_Msk = 1u << 0;
    static const constexpr uint32_t DACCVIOL_Msk = 1u << 1;
    static const constexpr uint32_t MSTKERR_Msk = 1u << 4;
    static const constexpr uint32_t MUNSTKERR_Msk = 1u << 3;
    static const constexpr uint32_t MLSPERR_Msk = 1u << 5;

    static const constexpr char *IACCVIOL_Txt = "Fault on instruction access";
    static const constexpr char *DACCVIOL_Txt = "Fault on direct data access";
    static const constexpr char *MSTKERR_Txt = "Context stacking, because of an MPU access violation";
    static const constexpr char *MUNSTKERR_Txt = "Context unstacking, because of an MPU access violation";
    static const constexpr char *MLSPERR_Txt = "During lazy floating-point state preservation";

    static const constexpr uint32_t MMARVALID_Msk = 1 << 7; // MemManage Fault Address Register (MMFAR) valid flag:

    static const constexpr uint32_t STKERR_Msk = 1u << 12;
    static const constexpr uint32_t UNSTKERR_Msk = 1u << 11;
    static const constexpr uint32_t IBUSERR_Msk = 1u << 8;
    static const constexpr uint32_t LSPERR_Msk = 1u << 13;
    static const constexpr uint32_t PRECISERR_Msk = 1u << 9;
    static const constexpr uint32_t IMPRECISERR_Msk = 1u << 10;

    static const constexpr char *STKERR_Txt = "During exception stacking";
    static const constexpr char *UNSTKERR_Txt = "During exception unstacking";
    static const constexpr char *IBUSERR_Txt = "During instruction prefetching, precise";
    static const constexpr char *LSPERR_Txt = "During lazy floating-point state preservation";
    static const constexpr char *PRECISERR_Txt = "Precise data access error, precise";
    static const constexpr char *IMPRECISERR_Txt = "Imprecise data access error, imprecise";

    static const constexpr uint32_t BFARVALID_Msk = 1U << 15; // MemManage Fault Address Register (MMFAR) valid flag:

    static const constexpr uint32_t UNDEFINSTR_Msk = 1u << 16;
    static const constexpr uint32_t INVSTATE_Msk = 1u << 17;
    static const constexpr uint32_t INVPC_Msk = 1u << 18;
    static const constexpr uint32_t NOCPC_Msk = 1u << 19;
    static const constexpr uint32_t UNALIGNED_Msk = 1u << 24;
    static const constexpr uint32_t DIVBYZERO_Msk = 1u << 25;

    static const constexpr char *UNDEFINSTR_Txt = "Undefined instruction";
    static const constexpr char *INVSTATE_Txt = "Attempt to enter an invalid instruction set state";
    static const constexpr char *INVPC_Txt = "Failed integrity check on exception return";
    static const constexpr char *NOCPC_Txt = "Attempt to access a non-existing coprocessor";
    static const constexpr char *UNALIGNED_Txt = "Illegal unaligned load or store";
    static const constexpr char *DIVBYZERO_Txt = "Divide By 0";
    //static const constexpr uint8_t STKOF = 1U << 0;

    static const constexpr uint8_t ROWS = 21;
    static const constexpr uint8_t COLS = 32;

    char tskName[configMAX_TASK_NAME_LEN];
    memset(tskName, '\0', sizeof(tskName) * sizeof(char)); // set to zeros to be on the safe side

    uint32_t __pxCurrentTCB;
    dump_in_xflash_read_RAM(&__pxCurrentTCB, (unsigned int)&pxCurrentTCB, sizeof(uint32_t));
    TCB_t CurrentTCB;
    dump_in_xflash_read_RAM(&CurrentTCB, __pxCurrentTCB, sizeof(TCB_t));

    strlcpy(tskName, CurrentTCB.pcTaskName, sizeof(tskName));
    StackType_t *pTopOfStack = (StackType_t *)CurrentTCB.pxTopOfStack;
    StackType_t *pBotOfStack = CurrentTCB.pxStack;

    display::Clear(COLOR_NAVY); //clear with dark blue color

    int buffer_size = COLS * ROWS + 1; ///< 7 bit ASCII allowed only (no UTF8)
    /// Buffer for text. PNG RAM cannot be used (font drawing).
    char buffer[buffer_size];
    int buffer_pos = 0; ///< position in buffer

    addFormatText(buffer, buffer_size, buffer_pos, "TASK: %s. ", tskName);

    uint32_t __SCB[35];
    dump_in_xflash_read_regs_SCB(&__SCB, 35 * sizeof(uint32_t));

    uint32_t __CFSR = __SCB[0x28 >> 2];

    switch ((__CFSR) & (IACCVIOL_Msk | DACCVIOL_Msk | MSTKERR_Msk | MUNSTKERR_Msk | MLSPERR_Msk | STKERR_Msk | UNSTKERR_Msk | IBUSERR_Msk | LSPERR_Msk | PRECISERR_Msk | IMPRECISERR_Msk | UNDEFINSTR_Msk | INVSTATE_Msk | INVPC_Msk | NOCPC_Msk | UNALIGNED_Msk | DIVBYZERO_Msk)) {
    case IACCVIOL_Msk:
        addText(buffer, buffer_size, buffer_pos, IACCVIOL_Txt);
        break;
    case DACCVIOL_Msk:
        addText(buffer, buffer_size, buffer_pos, DACCVIOL_Txt);
        break;
    case MSTKERR_Msk:
        addText(buffer, buffer_size, buffer_pos, MSTKERR_Txt);
        break;
    case MUNSTKERR_Msk:
        addText(buffer, buffer_size, buffer_pos, MUNSTKERR_Txt);
        break;
    case MLSPERR_Msk:
        addText(buffer, buffer_size, buffer_pos, MLSPERR_Txt);
        break;

    case STKERR_Msk:
        addText(buffer, buffer_size, buffer_pos, STKERR_Txt);
        break;
    case UNSTKERR_Msk:
        addText(buffer, buffer_size, buffer_pos, UNSTKERR_Txt);
        break;
    case IBUSERR_Msk:
        addText(buffer, buffer_size, buffer_pos, IBUSERR_Txt);
        break;
    case LSPERR_Msk:
        addText(buffer, buffer_size, buffer_pos, LSPERR_Txt);
        break;
    case PRECISERR_Msk:
        addText(buffer, buffer_size, buffer_pos, PRECISERR_Txt);
        break;
    case IMPRECISERR_Msk:
        addText(buffer, buffer_size, buffer_pos, IMPRECISERR_Txt);
        break;

    case UNDEFINSTR_Msk:
        addText(buffer, buffer_size, buffer_pos, UNDEFINSTR_Txt);
        break;
    case INVSTATE_Msk:
        addText(buffer, buffer_size, buffer_pos, INVSTATE_Txt);
        break;
    case INVPC_Msk:
        addText(buffer, buffer_size, buffer_pos, INVPC_Txt);
        break;
    case NOCPC_Msk:
        addText(buffer, buffer_size, buffer_pos, NOCPC_Txt);
        break;
    case UNALIGNED_Msk:
        addText(buffer, buffer_size, buffer_pos, UNALIGNED_Txt);
        break;
    case DIVBYZERO_Msk:
        addText(buffer, buffer_size, buffer_pos, DIVBYZERO_Txt);
        break;

    default:
        addFormatNum(buffer, buffer_size, buffer_pos, "Multiple Errors CFSR :%08x", __CFSR);
        break;
    }
    addText(buffer, buffer_size, buffer_pos, "\n");

    addFormatNum(buffer, buffer_size, buffer_pos, "bot: 0x%08x ", (uint32_t)pBotOfStack);
    addFormatNum(buffer, buffer_size, buffer_pos, "top: 0x%08x\n", (uint32_t)pTopOfStack);

    uint32_t __CPUID = __SCB[0x00 >> 2];
    uint32_t __ICSR = __SCB[0x04 >> 2];
    uint32_t __VTOR = __SCB[0x08 >> 2];
    uint32_t __AIRCR = __SCB[0x0c >> 2];
    uint32_t __SCR = __SCB[0x10 >> 2];
    uint32_t __CCR = __SCB[0x14 >> 2];
    uint32_t __SHCSR = __SCB[0x24 >> 2];
    uint32_t __HFSR = __SCB[0x2c >> 2];
    uint32_t __DFSR = __SCB[0x30 >> 2];
    uint32_t __MMFAR = __SCB[0x34 >> 2];
    uint32_t __BFAR = __SCB[0x38 >> 2];
    uint32_t __AFSR = __SCB[0x3c >> 2];
    uint32_t __DFR = __SCB[0x48 >> 2];
    uint32_t __ADR = __SCB[0x4c >> 2];
    uint32_t __CPACR = __SCB[0x88 >> 2];

    //32 characters per line
    addFormatNum(buffer, buffer_size, buffer_pos, "CPUID:%08x  ", __CPUID);
    if (__ICSR)
        addFormatNum(buffer, buffer_size, buffer_pos, "ICSR :%08x  ", __ICSR);
    if (__VTOR)
        addFormatNum(buffer, buffer_size, buffer_pos, "VTOR :%08x  ", __VTOR);
    if (__AIRCR)
        addFormatNum(buffer, buffer_size, buffer_pos, "AIRCR:%08x  ", __AIRCR);
    if (__SCR)
        addFormatNum(buffer, buffer_size, buffer_pos, "SCR  :%08x  ", __SCR);
    if (__CCR)
        addFormatNum(buffer, buffer_size, buffer_pos, "CCR  :%08x  ", __CCR);
    if (__SHCSR)
        addFormatNum(buffer, buffer_size, buffer_pos, "SHCSR:%08x  ", __SHCSR);
    if (__HFSR)
        addFormatNum(buffer, buffer_size, buffer_pos, "HFSR :%08x  ", __HFSR);
    if (__DFSR)
        addFormatNum(buffer, buffer_size, buffer_pos, "DFSR :%08x  ", __DFSR);
    if ((__CFSR)&MMARVALID_Msk)
        addFormatNum(buffer, buffer_size, buffer_pos, "MMFAR:%08x  ", __MMFAR); ///< print this only if value is valid
    if ((__CFSR)&BFARVALID_Msk)
        addFormatNum(buffer, buffer_size, buffer_pos, "BFAR :%08x  ", __BFAR); ///< print this only if value is valid
    if (__AFSR)
        addFormatNum(buffer, buffer_size, buffer_pos, "AFSR :%08x  ", __AFSR);
    if (__DFR)
        addFormatNum(buffer, buffer_size, buffer_pos, "DFR  :%08x  ", __DFR);
    if (__ADR)
        addFormatNum(buffer, buffer_size, buffer_pos, "ADR  :%08x  ", __ADR);
    if (__CPACR)
        addFormatNum(buffer, buffer_size, buffer_pos, "CPACR:%08x\n", __CPACR);

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

    const int lines = str2multiline(buffer, buffer_size, COLS);
    const int available_rows = lines < 0 ? 0 : ROWS - lines - 1;
    //int available_chars = available_rows * COLS;
    const int stack_sz = pTopOfStack - pBotOfStack;
    //int stack_chars_to_print = (addr_string_len +1)* stack_sz - stack_sz / 3;//+1 == space, - stack_sz / 3 .. 3rd string does not have a space
    const int requested_rows = stack_sz / 3; ///< 3 addresses per line

    StackType_t *lastAddr;
    if (requested_rows < available_rows)
        lastAddr = pBotOfStack - 1;
    else
        lastAddr = pTopOfStack - available_rows * strings_per_row;

    int space_counter = 0; //3rd string does not have a space behind it
    for (StackType_t *i = pTopOfStack; i != lastAddr; --i) {
        space_counter++;
        uint32_t sp = 0;
        dump_in_xflash_read_RAM(&sp, (unsigned int)i, sizeof(uint32_t));
        addFormatNum(buffer, buffer_size, buffer_pos, "0x%08x ", sp);
    }

    render_text_align(Rect16(8, 10, 232, 290), string_view_utf8::MakeCPUFLASH((const uint8_t *)buffer), resource_font(IDR_FNT_SMALL), COLOR_NAVY, COLOR_WHITE, { 0, 0, 0, 0 }, { Align_t::LeftTop(), is_multiline::yes });
    display::DrawText(Rect16(8, 290, 220, 20), string_view_utf8::MakeCPUFLASH((const uint8_t *)project_version_full), resource_font(IDR_FNT_SMALL), COLOR_NAVY, COLOR_WHITE);
}

#endif //PSOD_BSOD
