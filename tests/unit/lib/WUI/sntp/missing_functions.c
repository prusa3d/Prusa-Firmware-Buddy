/*
 * Linker stubs, in the same spirit as the nhttp tests: just enough of the
 * system layer to run the SNTP client code single-threaded on the host.
 */
#include <stdlib.h>
#include <stdint.h>

#include <lwip/tcpip.h>

sys_mutex_t lock_tcpip_core;

void sys_mutex_lock(sys_mutex_t *mutex) {
    (void)mutex;
}

void sys_mutex_unlock(sys_mutex_t *mutex) {
    (void)mutex;
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

/*
 * Counted rather than ignored. On the firmware a failed lwIP assert only
 * log_criticals and execution continues (src/buddy/lwip.cpp), so nothing
 * observable goes wrong when we violate one of lwIP's contracts -- which makes
 * a silent stub here exactly the wrong shape. The tests assert this stays zero.
 */
unsigned lwip_assert_count = 0;

void lwip_platform_assert(const char *message, const char *file, int line) {
    (void)message;
    (void)file;
    (void)line;
    lwip_assert_count++;
}

void wui_lwip_assert_core_locked(void) {
}

void lwip_platform_log_error(const char *message) {
    (void)message;
}

err_t tcpip_try_callback(tcpip_callback_fn fn, void *ctx) {
    fn(ctx);
    return ERR_OK;
}

/* Normally provided by wui_api.cpp, which is too heavy to pull in. */
void sntp_set_system_time(uint32_t sec) {
    (void)sec;
}
