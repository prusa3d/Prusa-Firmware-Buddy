#include <semihosting/semihosting.hpp>

namespace semihosting {

static inline int32_t semihosting_call(int32_t R0, int32_t R1) {
    int32_t rc;
    __asm__ volatile(
        "mov r0, %1\n" /* move int R0 to register r0 */
        "mov r1, %2\n" /* move int R1 to register r1 */
        "bkpt #0xAB\n" /* thumb mode semihosting call */
        "mov %0, r0" /* move register r0 to int rc */
        : "=r"(rc)
        : "r"(R0), "r"(R1)
        : "r0", "r1", "ip", "lr", "memory", "cc");
    return rc;
}

int32_t sys_clock() {
    return semihosting_call(SYS_CLOCK, 0);
}

int32_t sys_close(int32_t handle) {
    return semihosting_call(SYS_CLOSE, (int32_t)&handle);
}

int32_t sys_elapsed(uint64_t *ticks) {
    return semihosting_call(SYS_ELAPSED, (int32_t)ticks);
}

int32_t sys_errno() {
    return semihosting_call(SYS_ERRNO, 0);
}

void sys_exit(int32_t reason) {
    semihosting_call(SYS_EXIT, reason);
    for (;;)
        ;
    return;
}

void sys_exit(uint32_t reason, int32_t code) {
    uint32_t args[] = {
        (uint32_t)reason,
        (uint32_t)code,
    };
    semihosting_call(SYS_EXIT_EXTENDED, (int32_t)args);
    for (;;)
        ;
    return;
}

int32_t sys_flen(int32_t handle) {
    return semihosting_call(SYS_FLEN, (int32_t)&handle);
}

int32_t sys_getcmdline(void *buf, uint32_t size) {
    uint32_t args[] = {
        (uint32_t)buf,
        (uint32_t)size,
    };
    return semihosting_call(SYS_GET_CMDLINE, (int32_t)args);
}

int32_t sys_heapinfo(heapinfo_block_t *block) {
    return semihosting_call(SYS_HEAPINFO, (int32_t)block);
}

int32_t sys_iserror(int32_t status) {
    return semihosting_call(SYS_ISERROR, (int32_t)&status);
}

int32_t sys_istty(int32_t handle) {
    return semihosting_call(SYS_ISTTY, (int32_t)&handle);
}

int32_t sys_open(const char *path, open_mode_t mode, uint32_t path_len) {
    uint32_t args[] = {
        (uint32_t)path,
        (uint32_t)mode,
        (uint32_t)path_len,
    };
    return semihosting_call(SYS_OPEN, (int32_t)args);
}

int32_t sys_read(int32_t handle, void *buf, uint32_t count) {
    uint32_t args[] = {
        (uint32_t)handle,
        (uint32_t)buf,
        (uint32_t)count,
    };
    return semihosting_call(SYS_READ, (int32_t)args);
}

int32_t sys_readc() {
    return semihosting_call(SYS_READC, 0);
}

int32_t sys_remove(char *path, uint32_t path_len) {
    uint32_t args[] = {
        (uint32_t)path,
        (uint32_t)path_len,
    };
    return semihosting_call(SYS_REMOVE, (int32_t)args);
}

int32_t sys_rename(char *old_path, uint32_t old_path_len, char *new_path, uint32_t new_path_len) {
    uint32_t args[] = {
        (uint32_t)old_path,
        (uint32_t)old_path_len,
        (uint32_t)new_path,
        (uint32_t)new_path_len,
    };
    return semihosting_call(SYS_RENAME, (int32_t)args);
}

int32_t sys_seek(int32_t handle, int32_t pos) {
    uint32_t args[] = {
        (uint32_t)handle,
        (uint32_t)pos,
    };
    return semihosting_call(SYS_SEEK, (int32_t)args);
}

int32_t sys_system(const char *command, uint32_t length) {
    uint32_t args[] = {
        (uint32_t)command,
        (uint32_t)length,
    };
    return semihosting_call(SYS_SYSTEM, (int32_t)args);
}

int32_t sys_tickfreq() {
    return semihosting_call(SYS_TICKFREQ, 0);
}

int32_t sys_time() {
    return semihosting_call(SYS_TIME, 0);
}

int32_t sys_tmpnam(void *buf, int32_t target_id, uint32_t buf_size) {
    uint32_t args[] = {
        (uint32_t)buf,
        (uint32_t)target_id,
        (uint32_t)buf_size,
    };
    return semihosting_call(SYS_TMPNAM, (int32_t)args);
}

int32_t sys_write(int32_t handle, void *buf, uint32_t count) {
    uint32_t args[] = {
        (uint32_t)handle,
        (uint32_t)buf,
        (uint32_t)count,
    };
    return semihosting_call(SYS_WRITE, (int32_t)args);
}

void sys_writec(char ch) {
    semihosting_call(SYS_WRITEC, (int32_t)&ch);
}

void sys_write0(char *str) {
    semihosting_call(SYS_WRITE0, (int32_t)str);
}

} // namespace semihosting
