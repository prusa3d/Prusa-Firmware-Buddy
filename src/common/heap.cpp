// Simplified and customized version based on a code from article below.
// http://www.nadler.com/embedded/newlibAndFreeRTOS.html
#include <stdlib.h>
#include <sys/reent.h>
#include <stdbool.h>
#include <malloc.h>
#include <errno.h>
#include <stddef.h>
#include "newlib.h"
#include "bsod.h"
#include "heap.h"
#include "FreeRTOS.h"
#include "cmsis_os.h"

#if !defined(configUSE_NEWLIB_REENTRANT) || (configUSE_NEWLIB_REENTRANT != 1)
    #warning "#define configUSE_NEWLIB_REENTRANT 1"
#endif
#define ISR_STACK_LENGTH_BYTES 512 // #define bytes to reserve for ISR (MSP) stack

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
        vApplicationMallocFailedHook();
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
        bsod_nofn_noln("_dtoa_r (float formatting) called from ISR");
    }
    return __real__dtoa_r(r, a, b, c, d, e, f);
}
