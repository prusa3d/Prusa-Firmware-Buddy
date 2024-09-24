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
 * Ideas for better testing fixture are welcome, but for now we have built our
 * server on top of altcp, so we have to bring LwIP in.
 */
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <lwip/tcpip.h>
#include <cmsis_os.h>

#include "../../../src/common/filepath_operation.h"

size_t strlcpy(char *, const char *, size_t);

size_t strlcat(char *dst, const char *src, size_t size) {
    /*
     * Note: this is not _completely_ correct. Specifically, if dst is longer
     * than size, it does bad things. Good enough for test purposes.
     */
    const size_t start = strlen(dst);
    strncat(dst + start, src, size - 1 - start);
    return start + strlen(src);
}

sys_mutex_t lock_tcpip_core;

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

err_t tcpip_callback(tcpip_callback_fn fn, void *ctx) {
    fn(ctx);
    return ERR_OK;
}

void get_LFN(char *lfn, size_t lfn_size, char *path) {
    strlcpy(lfn, basename_b(path), lfn_size);
}

void mbedtls_platform_zeroize(void *b, size_t size) {
    memset(b, 0, size);
}

bool wui_is_file_being_printed(const char *filename) {
    return false;
}

int mkdir(const char *path, mode_t mode) {
    return 0;
}

void get_SFN_path(char *path) {
    // Stays intact
}

void get_SFN_path_copy(const char *lfn, char *sfn_out, size_t size) {
    strlcpy(sfn_out, lfn, size);
}

uint32_t osDelay(uint32_t time) {
    return 0;
}

void sys_mutex_lock(sys_mutex_t *mutex) {
}

void sys_mutex_unlock(sys_mutex_t *mutex) {
}

void lwip_platform_assert(const char *message, const char *file, int line) {
}

void wui_lwip_assert_core_locked() {
}

void lwip_platform_log_error(const char *message) {
}
