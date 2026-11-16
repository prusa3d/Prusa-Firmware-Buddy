/// @file
#include "heap.h"
#include "heap.hpp"

// Simplified and customized version based on a code from article below.
// http://www.nadler.com/embedded/newlibAndFreeRTOS.html
#include <atomic>
#include <stdlib.h>
#include <sys/reent.h>
#include <stdbool.h>
#include <malloc.h>
#include <cstddef>
#include <errno.h>
#include <stddef.h>
#include <string.h>
#include "newlib.h"
#include "bsod.h"
#include "FreeRTOS.h"
#include "cmsis_os.h"
#include <option/board_is_master_board.h>

#if !defined(configUSE_NEWLIB_REENTRANT) || (configUSE_NEWLIB_REENTRANT != 1)
    // #error dead code found by automatic analyses (see BFW-5461)
    #warning "#define configUSE_NEWLIB_REENTRANT 1"
#endif

using std::atomic;

//
// FreeRTOS memory API
//

void *pvPortMalloc(size_t xSize) PRIVILEGED_FUNCTION {
    void *p = malloc(xSize);
    return p;
}
void vPortFree(void *pv) PRIVILEGED_FUNCTION {
    free(pv);
};

void vApplicationMallocFailedHook() {
    // This cannot be Red Screen as it can be called before GUI images are available
    bsod("Out of heap memory");
}

size_t xPortGetFreeHeapSize(void) PRIVILEGED_FUNCTION {
    struct mallinfo mi = mallinfo(); // available space now managed by newlib
    return mi.fordblks + heap_bytes_remaining(); // plus space not yet handed to newlib by sbrk
}

void vPortInitialiseBlocks(void) PRIVILEGED_FUNCTION {};

#define ENTER_CRITICAL_SECTION(_usis) \
    { _usis = taskENTER_CRITICAL_FROM_ISR(); } // Disables interrupts (after saving prior state)
#define EXIT_CRITICAL_SECTION(_usis) \
    { taskEXIT_CRITICAL_FROM_ISR(_usis); } // Re-enables interrupts (unless already disabled prior taskENTER_CRITICAL)

// We want to have a malloc_fallible (or calloc_fallible) on-demand. This is to
// "smuggle" the request into the _sbrk_r through several stack frames, denotes
// the task that requested it.
static atomic<TaskHandle_t> fallible_request_for = nullptr;

//
// _sbrk implementation
//

#if BOARD_IS_MASTER_BOARD()
    #define ISR_STACK_LENGTH_BYTES 2048 // #define bytes to reserve for ISR (MSP) stack
#else
    #define ISR_STACK_LENGTH_BYTES 512 // #define bytes to reserve for ISR (MSP) stack
#endif
static_assert(ISR_STACK_LENGTH_BYTES % alignof(std::max_align_t) == 0);

extern char heap_start[] __asm__("end");
extern char ram_end[] __asm__("_estack");

// Note: Compiler complains about going to negative ram_end indexes
// if we don't do the dance with the extra heap_start math
// Must be a function - global variables get initialized later than we need
inline char *heap_end() {
    return heap_start + (ram_end - heap_start) - ISR_STACK_LENGTH_BYTES;
}

// Must be a function - global variables get initialized later than we need
static char *&current_heap_end() {
    static char *val = heap_start;
    return val;
}

// Must be a function - global variables get initialized later than we need
extern "C" uint32_t heap_total_size() {
    return heap_end() - heap_start;
}

extern "C" uint32_t heap_bytes_remaining() {
    return heap_total_size() - (current_heap_end() - heap_start);
}

StackOverflowChecker &isr_stack_overflow_checker() {
    static StackOverflowChecker checker(std::as_writable_bytes(std::span { heap_end(), ram_end }));
    return checker;
}

void *_sbrk_r([[maybe_unused]] struct _reent *pReent, int incr) {
    UBaseType_t usis; // saved interrupt status

    ENTER_CRITICAL_SECTION(usis);
    char *previous_heap_end = current_heap_end();
    if (current_heap_end() + incr > heap_end()) {
        EXIT_CRITICAL_SECTION(usis);
        if (fallible_request_for.load() != xTaskGetCurrentTaskHandle()) {
            vApplicationMallocFailedHook();
        }
        return (char *)-1; // the malloc-family routine that called sbrk will return 0
    }
    // 'incr' of memory is available: update accounting and return it.
    current_heap_end() += incr;
    EXIT_CRITICAL_SECTION(usis);
    return (char *)previous_heap_end;
}

void *sbrk(int incr) { return _sbrk_r(_impure_ptr, incr); }

void *_sbrk(int incr) { return sbrk(incr); };

extern "C" void *malloc_fallible(size_t size) {
    TaskHandle_t me = xTaskGetCurrentTaskHandle();
    TaskHandle_t competitor = nullptr;

    while (!fallible_request_for.compare_exchange_strong(competitor, me, std::memory_order_acq_rel)) {
        // We have some other thread running a malloc_fallible request at this
        // very moment. Wait for it to finish.
        //
        // Such thing is very unlikely to happen in practice, since:
        // * Allocations are rare, fallible ones even more so.
        // * Currently, the only part that uses fallible allocations is mbedTLS
        // and it's used only from one thread.
        //
        // But we still need to have some way to deal with such situation ‒
        // busy-waiting is likely good enough.
        //
        // Some kind of "yield" to other threads would be great. But we would
        // need to deal with priorities, make sure the competitor is at least
        // as high prio as us and clean it up afterwards, which would be complex.
        osDelay(1);

        competitor = nullptr;
    }

    void *result = malloc(size);

    // Give up the slot.
    fallible_request_for.store(nullptr);

    return result;
}

extern "C" void *calloc_fallible(size_t nmemb, size_t size) {
    size_t total_size = nmemb * size;
    void *result = malloc_fallible(total_size);
    if (result != nullptr) {
        memset(result, 0, total_size);
    }
    return result;
}

//
// malloc_[un]lock implementation
//

static UBaseType_t malloc_saved_interrupt_status;
static int malloc_lock_counter = 0;

void __malloc_lock([[maybe_unused]] struct _reent *r) {
    UBaseType_t interrupt_status;
    ENTER_CRITICAL_SECTION(interrupt_status);
    if (malloc_lock_counter == 0) {
        malloc_saved_interrupt_status = interrupt_status;
    }
    malloc_lock_counter += 1;
};

void __malloc_unlock([[maybe_unused]] struct _reent *r) {
    malloc_lock_counter -= 1;
    if (malloc_lock_counter == 0) {
        EXIT_CRITICAL_SECTION(malloc_saved_interrupt_status);
    }
};

uint32_t mem_is_heap_allocated(const void *ptr) {
    return (ptr >= heap_start && ptr < heap_end);
}

//
// _dtoa_r wrap to ensure it is not being called from ISR
//

extern char *__real__dtoa_r(struct _reent *, double, int, int, int *, int *, char **);

char *__wrap__dtoa_r(struct _reent *r, double a, int b, int c, int *d, int *e, char **f) {
    if (xPortIsInsideInterrupt()) {
        _bsod("_dtoa_r (float formatting) called from ISR", "", 0);
    }
    return __real__dtoa_r(r, a, b, c, d, e, f);
}
