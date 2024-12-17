// Simplified and customized version based on a code from article below.
// http://www.nadler.com/embedded/newlibAndFreeRTOS.html
#include <atomic>
#include <stdlib.h>
#include <sys/reent.h>
#include <stdbool.h>
#include <malloc.h>
#include <errno.h>
#include <stddef.h>
#include <string.h>
#include "newlib.h"
#include "bsod.h"
#include "heap.h"
#include "FreeRTOS.h"
#include "cmsis_os.h"

#if !defined(configUSE_NEWLIB_REENTRANT) || (configUSE_NEWLIB_REENTRANT != 1)
    #warning "#define configUSE_NEWLIB_REENTRANT 1"
#endif
#define ISR_STACK_LENGTH_BYTES 512 // #define bytes to reserve for ISR (MSP) stack

using std::atomic;

uint32_t heap_total_size;
uint32_t heap_bytes_remaining;

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
    return mi.fordblks + heap_bytes_remaining; // plus space not yet handed to newlib by sbrk
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

#define __HeapBase  end
#define __HeapLimit _estack // except in K64F this was already adjusted in LD for stack...
extern char __HeapBase, __HeapLimit; // make sure to define these symbols in linker LD command file
register char *stack_ptr asm("sp");

void *_sbrk_r([[maybe_unused]] struct _reent *pReent, int incr) {
    UBaseType_t usis; // saved interrupt status
    static char *current_heap_end = &__HeapBase;
    if (heap_total_size == 0) {
        heap_total_size = heap_bytes_remaining = (int)((&__HeapLimit) - (&__HeapBase)) - ISR_STACK_LENGTH_BYTES;
    };
    char *limit = (xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED) ? stack_ptr : // Before scheduler is started, limit is stack pointer (risky!)
        &__HeapLimit - ISR_STACK_LENGTH_BYTES; // Once running, OK to reuse all remaining RAM except ISR stack (MSP) stack
    ENTER_CRITICAL_SECTION(usis);
    char *previous_heap_end = current_heap_end;
    if (current_heap_end + incr > limit) {
        EXIT_CRITICAL_SECTION(usis);
        if (fallible_request_for.load() != xTaskGetCurrentTaskHandle()) {
            vApplicationMallocFailedHook();
        }
        return (char *)-1; // the malloc-family routine that called sbrk will return 0
    }
    // 'incr' of memory is available: update accounting and return it.
    current_heap_end += incr;
    heap_bytes_remaining -= incr;
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
    return (ptr >= (void *)&__HeapBase && ptr < (void *)&__HeapLimit);
}

//
// _dtoa_r wrap to ensure it is not being called from ISR
//

extern char *__real__dtoa_r(struct _reent *, double, int, int, int *, int *, char **);

char *__wrap__dtoa_r(struct _reent *r, double a, int b, int c, int *d, int *e, char **f) {
    if (xPortIsInsideInterrupt()) {
        _bsod("_dtoa_r (float formatting) called from ISR", 0, 0);
    }
    return __real__dtoa_r(r, a, b, c, d, e, f);
}
