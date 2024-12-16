// bsod_gui.cpp - blue screen of death
#include "bsod.h"
#include "bsod_gui.hpp"
#include "display.hpp"
#include <find_error.hpp>
#include "wdt.hpp"
#include <crash_dump/dump.hpp>
#include "safe_state.h"

#include <crash_dump/dump.hpp>
#include <guiconfig/guiconfig.h>

#include "FreeRTOS.h"
#include "task.h"
#include "led_animations/animation.hpp"

#include <iterator>
#include <stdarg.h>

#include "sound.hpp"
#include "gui.hpp"
#include "Jogwheel.hpp"
#include "sys.h"
#include "hwio.h"
#include <version/version.hpp>
#include "support_utils.h"
#include "error_codes.hpp"
#include "../../lib/Marlin/Marlin/src/core/language.h"
#include "power_panic.hpp"
#include "crash_dump/dump_parse.hpp"

#include "str_utils.hpp"

// this is private struct definition from FreeRTOS
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
    char pcTaskName[configMAX_TASK_NAME_LEN]; /*< Descriptive name given to the task when created.  Facilitates debugging only. */
    /*lint !e971 Unqualified char types are allowed for strings and single characters only. */

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

// current thread from FreeRTOS
extern PRIVILEGED_DATA TCB_t *volatile pxCurrentTCB;

void raise_redscreen(ErrCode error_code, const char *error, const char *module) {
    crash_dump::save_message(crash_dump::MsgType::RSOD, ftrstd::to_underlying(error_code), error, module);
    sys_reset();
}

[[noreturn]] void fatal_error(const ErrCode error_code, ...) {
    const ErrDesc &corresponding_error = find_error(error_code);
    ArrayStringBuilder<140> str_builder;
    va_list args;
    va_start(args, error_code);
    str_builder.append_vprintf(corresponding_error.err_text, args);
    va_end(args);
    raise_redscreen(error_code, str_builder.str(), corresponding_error.err_title);
}

//! Fatal error that causes Redscreen
void fatal_error(const char *error, const char *module) {
    // Unknown error code = we don't have prusa.io/###### site support for this error
    // In this case we have to dump error message and error title
    ErrCode error_code = ErrCode::ERR_UNDEF;

    /// Decision tree to define error code
    using namespace Language_en;
    /// TODO share these strings (saves ~100 B of binary size)
    if (strcmp("Emergency stop (M112)", error) == 0) {
        fatal_error(ErrCode::ERR_SYSTEM_EMERGENCY_STOP);
    } else if (strcmp(MSG_HEATING_FAILED_LCD, error) == 0) {
        fatal_error(ErrCode::ERR_TEMPERATURE_HOTEND_PREHEAT_ERROR);
    } else if (strcmp(MSG_THERMAL_RUNAWAY, error) == 0) {
        fatal_error(ErrCode::ERR_TEMPERATURE_HOTEND_THERMAL_RUNAWAY);
    } else if (strcmp(MSG_ERR_MAXTEMP, error) == 0) {
        fatal_error(ErrCode::ERR_TEMPERATURE_HOTEND_MAXTEMP_ERROR);
    } else if (strcmp(MSG_ERR_MINTEMP, error) == 0) {
        fatal_error(ErrCode::ERR_TEMPERATURE_HOTEND_MINTEMP_ERROR);
#if !PRINTER_IS_PRUSA_XL()
    } else if (strcmp(MSG_HEATING_FAILED_LCD_BED, error) == 0) {
        fatal_error(ErrCode::ERR_TEMPERATURE_BED_PREHEAT_ERROR);
    } else if (strcmp(MSG_THERMAL_RUNAWAY_BED, error) == 0) {
        fatal_error(ErrCode::ERR_TEMPERATURE_BED_THERMAL_RUNAWAY);
    } else if (strcmp(MSG_ERR_MAXTEMP_BED, error) == 0) {
        fatal_error(ErrCode::ERR_TEMPERATURE_BED_MAXTEMP_ERROR);
    } else if (strcmp(MSG_ERR_MINTEMP_BED, error) == 0) {
        fatal_error(ErrCode::ERR_TEMPERATURE_BED_MINTEMP_ERROR);
#endif // !PRINTER_IS_PRUSA_XL()
#if !PRINTER_IS_PRUSA_MK3_5()
    } else if (strcmp(MSG_ERR_MINTEMP_HEATBREAK, error) == 0) {
        fatal_error(ErrCode::ERR_TEMPERATURE_HEATBREAK_MINTEMP_ERR);
    } else if (strcmp(MSG_ERR_MAXTEMP_HEATBREAK, error) == 0) {
        fatal_error(ErrCode::ERR_TEMPERATURE_HEATBREAK_MAXTEMP_ERR);
#endif
    }
#if PRINTER_IS_PRUSA_XL()
    else if (strcmp(MSG_ERR_NOZZLE_OVERCURRENT, error) == 0) {
        fatal_error(ErrCode::ERR_ELECTRO_HEATER_HOTEND_OVERCURRENT, module);
    }
#endif

    // error code is not defined, raise redscreen with custom error message and error title
    raise_redscreen(error_code, error, module);
}

/**
 * @brief Put HW into safe state, activate display safe mode and initialize it twice
 * @note Cannot be done from high priority ISR.
 */
static void stop_common(void) {
    hwio_safe_state();

    display::enable_safe_mode();

    hwio_beeper_notone();
    display::init();
    display::init();
}

/**
 * @brief Show simplified fallback BSOD screen.
 * Used if new BSOD happens before previous BSOD can be shown.
 * @param fmt format string with fallback bsod message
 * @param file_name file name where bsod happened
 * @param line_number line number where bsod happened
 * @param args arguments for fmt
 */
static void fallback_bsod(const char *fmt, const char *file_name, int line_number, va_list args) {
    // Set BSOD as displayed to prevent loop
    crash_dump::message_set_displayed();
    crash_dump::dump_set_displayed();

    // Disable after clearing dump flags
    __disable_irq();

    // Stop most HW
    stop_common();

    ///< Clear with dark blue color
    display::clear(COLOR_NAVY);

    char fallback_bsod_text[300];

    // Add filename to buffer
    size_t consumed = snprintf(fallback_bsod_text, std::size(fallback_bsod_text), "Fallback BSOD\n(possibly BSODception)\n%s\n%s:%d\n",
        version::project_version_full, file_name, line_number);

    // Add message to buffer
    if (consumed < std::size(fallback_bsod_text)) {
        vsnprintf(fallback_bsod_text + consumed, std::size(fallback_bsod_text) - consumed, fmt, args);
    }

    // Draw buffer
    render_text_align(Rect16(8, 10, 230, 290),
        string_view_utf8::MakeRAM((const uint8_t *)fallback_bsod_text), Font::small, COLOR_NAVY, COLOR_WHITE,
        { 0, 0, 0, 0 }, { Align_t::LeftTop(), is_multiline::yes });

    // Endless loop
    while (1) {
        wdt_iwdg_refresh();
    }
}

static bool use_fallback_bsod = true; ///< Use fallback BSOD before guimain is able to show a proper BSOD
void bsod_mark_shown() {
    use_fallback_bsod = false; // BSOD would be shown now, can dump a new one
}

void _bsod(const char *fmt, const char *file_name, int line_number, ...) {
    va_list args;
    va_start(args, line_number);

    // Check recursive bsod, can happen if bsod happens during processing of a previous bsod
    if (use_fallback_bsod) {
        // There could already be a dump that was not displayed
        fallback_bsod(fmt, file_name, line_number, args);
    }

    // Get file and line as title
    char title[crash_dump::MSG_TITLE_MAX_LEN];
    snprintf(title, std::size(title), "%s:%d", file_name, line_number);

    // Get message
    char msg[crash_dump::MSG_MAX_LEN];
    vsnprintf(msg, std::size(msg), fmt, args);
    va_end(args);

    // Save file, line and meessage
    crash_dump::save_message(crash_dump::MsgType::BSOD, ftrstd::to_underlying(ErrCode::ERR_UNDEF), msg, title);

    crash_dump::trigger_crash_dump();
}

#ifdef configCHECK_FOR_STACK_OVERFLOW

extern "C" void vApplicationStackOverflowHook([[maybe_unused]] TaskHandle_t xTask, char *pcTaskName) {
    // Save task name as title
    crash_dump::save_message(crash_dump::MsgType::STACK_OVF, 0, "", reinterpret_cast<char *>(pcTaskName));

    crash_dump::trigger_crash_dump();
}

#endif // configCHECK_FOR_STACK_OVERFLOW

namespace bsod_details {

const char *get_hardfault_reason() {
    // https://www.freertos.org/Debugging-Hard-Faults-On-Cortex-M-Microcontrollers.html

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

    static const constexpr uint32_t IACCVIOL_Msk = 1u << 0;
    static const constexpr uint32_t DACCVIOL_Msk = 1u << 1;
    static const constexpr uint32_t MSTKERR_Msk = 1u << 4;
    static const constexpr uint32_t MUNSTKERR_Msk = 1u << 3;
    static const constexpr uint32_t MLSPERR_Msk = 1u << 5;
    static const constexpr uint32_t STKERR_Msk = 1u << 12;
    static const constexpr uint32_t UNSTKERR_Msk = 1u << 11;
    static const constexpr uint32_t IBUSERR_Msk = 1u << 8;
    static const constexpr uint32_t LSPERR_Msk = 1u << 13;
    static const constexpr uint32_t PRECISERR_Msk = 1u << 9;
    static const constexpr uint32_t IMPRECISERR_Msk = 1u << 10;
    static const constexpr uint32_t UNDEFINSTR_Msk = 1u << 16;
    static const constexpr uint32_t INVSTATE_Msk = 1u << 17;
    static const constexpr uint32_t INVPC_Msk = 1u << 18;
    static const constexpr uint32_t NOCPC_Msk = 1u << 19;
    static const constexpr uint32_t UNALIGNED_Msk = 1u << 24;
    static const constexpr uint32_t DIVBYZERO_Msk = 1u << 25;

    crash_dump::CrashCatcherDumpParser parser;
    uint8_t scg_regs_data[sizeof(SCB_Type)];
    parser.load_data(scg_regs_data, reinterpret_cast<uintptr_t>(SCB), sizeof(*SCB));
    SCB_Type *scg_regs = reinterpret_cast<SCB_Type *>(&scg_regs_data);
    const uint32_t cfsr = scg_regs->CFSR;

    switch ((cfsr) & (IACCVIOL_Msk | DACCVIOL_Msk | MSTKERR_Msk | MUNSTKERR_Msk | MLSPERR_Msk | STKERR_Msk | UNSTKERR_Msk | IBUSERR_Msk | LSPERR_Msk | PRECISERR_Msk | IMPRECISERR_Msk | UNDEFINSTR_Msk | INVSTATE_Msk | INVPC_Msk | NOCPC_Msk | UNALIGNED_Msk | DIVBYZERO_Msk)) {
        // case ????_Msk:
        // urn "The error can have at most 38 chars -|"; // To fit into title
    case IACCVIOL_Msk:
        return "Fault on instruction access";
    case DACCVIOL_Msk:
        return "Fault on direct data access";
    case MSTKERR_Msk:
        return "Context stacking, (MPU access)";
    case MUNSTKERR_Msk:
        return "Context unstacking, (MPU access)";
    case MLSPERR_Msk:
        return "During lazy FP state preservation (M)";
    case STKERR_Msk:
        return "During exception stacking";
    case UNSTKERR_Msk:
        return "During exception unstacking";
    case IBUSERR_Msk:
        return "During instr prefetching, precise";
    case LSPERR_Msk:
        return "During lazy FP state preservation";
    case PRECISERR_Msk:
        return "Precise data access error, precise";
    case IMPRECISERR_Msk:
        return "Imprecise data access error, imprecise";
    case UNDEFINSTR_Msk:
        return "Undefined instruction";
    case INVSTATE_Msk:
        return "Enter an invalid instruction set state";
    case INVPC_Msk:
        return "Integrity check on exception return";
    case NOCPC_Msk:
        return "Access a non-existing coprocessor";
    case UNALIGNED_Msk:
        return "Illegal unaligned load or store";
    case DIVBYZERO_Msk:
        return "Divide By 0";

    default: {
        static char buffer[39];
        snprintf(buffer, std::size(buffer), "Multiple Errors CFSR :%08x", static_cast<unsigned int>(cfsr));
        return buffer;
    }
    }
}

size_t get_task_name(char *&buffer, size_t buffer_size) {
    crash_dump::CrashCatcherDumpParser parser;

    // Get task name from dump
    uintptr_t __pxCurrentTCB;
    parser.load_data((uint8_t *)(&__pxCurrentTCB), (uintptr_t)(&pxCurrentTCB), sizeof(uint32_t));
    TCB_t CurrentTCB;
    parser.load_data(reinterpret_cast<uint8_t *>(&CurrentTCB), reinterpret_cast<uintptr_t>(__pxCurrentTCB), sizeof(TCB_t));

    // Add to buffer
    int written = snprintf(buffer, buffer_size, "task:%s\n", CurrentTCB.pcTaskName);

    // Check that it fits to buffer
    if (written >= 0 && static_cast<size_t>(written) < buffer_size) {
        buffer += written;
        return buffer_size - written;
    } else {
        buffer = nullptr;
        return buffer_size;
    }
}

size_t get_regs(char *&buffer, size_t buffer_size) {
    crash_dump::CrashCatcherDumpParser parser;

    crash_dump::CrashCatcherDumpParser::Registers_t gen_regs;
    parser.load_registers(gen_regs);

    uint8_t scg_regs_data[sizeof(SCB_Type)];
    parser.load_data(scg_regs_data, reinterpret_cast<uintptr_t>(SCB), sizeof(*SCB));
    SCB_Type *scg_regs = reinterpret_cast<SCB_Type *>(&scg_regs_data);
    const uint32_t cfsr = scg_regs->CFSR;

    auto add_reg = [&](const char *name, unsigned int value, bool even_if_zero = false) {
        // Ignore if it is empty or out of space in buffer
        if (buffer == nullptr || buffer_size == 0
            || (!even_if_zero && value == 0)) {
            return true;
        }

        // Add to buffer
        int written = snprintf(buffer, buffer_size, "%s:%08x ", name, value);

        // Check that it fits to buffer
        if (written >= 0 && static_cast<size_t>(written) < buffer_size) {
            buffer += written;
            buffer_size -= written;
            return true;
        } else {
            buffer_size = 0;
            buffer = nullptr;
            return false;
        }
    };

    // Core registers
    add_reg("SP", gen_regs.SP, true);
    add_reg("LR", gen_regs.LR, true);
    add_reg("PC", gen_regs.PC, true);

    // Add newline at the end
    if (buffer_size > 1) {
        *buffer = '\n';
        ++buffer;
        *buffer = '\0';
    }

    // SCB registers
    add_reg("CPUID", scg_regs->CPUID);
    add_reg("ICSR_", scg_regs->ICSR);
    add_reg("VTOR_", scg_regs->VTOR);
    add_reg("AIRCR", scg_regs->AIRCR);
    add_reg("SCR__", scg_regs->SCR);
    add_reg("CCR__", scg_regs->CCR);
    add_reg("SHCSR", scg_regs->SHCSR);
    add_reg("HFSR_", scg_regs->HFSR);
    add_reg("DFSR_", scg_regs->DFSR);
    // Print these only if value is valid
    static const constexpr uint32_t MMARVALID_Msk = 1 << 7; ///< MemManage Fault Address Register (MMFAR) valid flag
    if (cfsr & MMARVALID_Msk) {
        add_reg("MMFAR", scg_regs->MMFAR);
    }
    static const constexpr uint32_t BFARVALID_Msk = 1U << 15; ///< BusFault Address Register (BFAR) valid flag
    if (cfsr & BFARVALID_Msk) {
        add_reg("BFAR_", scg_regs->BFAR);
    }
    add_reg("AFSR_", scg_regs->AFSR);
    add_reg("DFR__", scg_regs->DFR);
    add_reg("ADR__", scg_regs->ADR);
    add_reg("CPACR", scg_regs->CPACR);

    // Add newline at the end
    if (buffer_size > 1) {
        *buffer = '\n';
        ++buffer;
        *buffer = '\0';
    }

    return buffer_size;
}

size_t get_stack(char *&buffer, size_t buffer_size) {
    crash_dump::CrashCatcherDumpParser parser;

    // Get stack from dump
    uintptr_t __pxCurrentTCB;
    parser.load_data((uint8_t *)(&__pxCurrentTCB), (uintptr_t)(&pxCurrentTCB), sizeof(uint32_t));
    TCB_t CurrentTCB;
    parser.load_data(reinterpret_cast<uint8_t *>(&CurrentTCB), reinterpret_cast<uintptr_t>(__pxCurrentTCB), sizeof(TCB_t));

    StackType_t *pTopOfStack = (StackType_t *)CurrentTCB.pxTopOfStack;
    StackType_t *pBotOfStack = CurrentTCB.pxStack;

    // Add location to buffer
    int written = snprintf(buffer, buffer_size, "bot:%08x top:%08x stack:\n", reinterpret_cast<unsigned int>(pTopOfStack), reinterpret_cast<unsigned int>(pBotOfStack));

    // Check that it fits to buffer
    if (written >= 0 && static_cast<size_t>(written) < buffer_size) {
        buffer += written;
        buffer_size -= written;
    } else {
        buffer_size = 0;
        buffer = nullptr;
    }

    for (StackType_t *i = pTopOfStack; i > pBotOfStack && buffer && buffer_size; --i) {
        // Get stack content
        uint32_t sp = 0;
        parser.load_data(reinterpret_cast<uint8_t *>(&sp), reinterpret_cast<uintptr_t>(i), sizeof(uint32_t));

        // Add content to buffer
        written = snprintf(buffer, buffer_size, "%08x ", static_cast<unsigned int>(sp));

        // Check that it fits to buffer
        if (written >= 0 && static_cast<size_t>(written) < buffer_size) {
            buffer += written;
            buffer_size -= written;
        } else {
            buffer_size = 0;
            buffer = nullptr;
        }
    };

    return buffer_size;
}

} // namespace bsod_details
