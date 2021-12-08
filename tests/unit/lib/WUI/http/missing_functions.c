/*
 * Bunch of test stubs.
 *
 * This is an desperate attempt to build the unit test code and the tested code
 * that uses LwIP extensively without bringing in all of the FreeRTOS and other
 * things. So we provide some stub functions here where it's less work than
 * bringing in a big dependency tree of C files.
 *
 * Note that these are very minimal and in many aspects often _not working_. We
 * suspect these are there just to satisfy the linker and won't actually be
 * called during the runtime, or, in case of the thread synchronization, won't
 * be needed because we have just one thread.
 *
 * But yes, this is ugly and hopefully won't be needed once we get some better
 * http server thing.
 */
#include <stdlib.h>
#include <stdint.h>
#include <lwip/tcpip.h>

void *mem_malloc(size_t size) {
    return malloc(size);
}

void mem_free(void *mem) {
    free(mem);
}

void *mem_trim(void *mem, size_t size) {
    (void)size;
    return mem;
}

int sys_arch_protect(void) {
    return 1;
}

void sys_arch_unprotect(int unused) {
    (void)unused;
}

uint32_t sys_now(void) {
    return 0;
}

err_t tcpip_try_callback(tcpip_callback_fn fn, void *ctx) {
    fn(ctx);
    return ERR_OK;
}
