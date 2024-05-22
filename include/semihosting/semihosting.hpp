#pragma once
#include <cstddef>
#include <cstdint>

namespace semihosting {

enum {
    SYS_CLOCK = 0x10,
    SYS_CLOSE = 0x02,
    SYS_ELAPSED = 0x30,
    SYS_ERRNO = 0x13,
    SYS_EXIT = 0x18,
    SYS_EXIT_EXTENDED = 0x20,
    SYS_FLEN = 0x0C,
    SYS_GET_CMDLINE = 0x15,
    SYS_HEAPINFO = 0x16,
    SYS_ISERROR = 0x08,
    SYS_ISTTY = 0x09,
    SYS_OPEN = 0x01,
    SYS_READ = 0x06,
    SYS_READC = 0x07,
    SYS_REMOVE = 0x0E,
    SYS_RENAME = 0x0F,
    SYS_SEEK = 0x0A,
    SYS_SYSTEM = 0x12,
    SYS_TICKFREQ = 0x31,
    SYS_TIME = 0x11,
    SYS_TMPNAM = 0x0D,
    SYS_WRITE = 0x05,
    SYS_WRITEC = 0x03,
    SYS_WRITE0 = 0x04,
};

typedef enum {
    OPEN_MODE_R = 0, /// read
    OPEN_MODE_RB = 1, /// read binary
    OPEN_MODE_RP = 2, /// read plus
    OPEN_MODE_RPB = 3, /// read plus binary
    OPEN_MODE_W = 4, /// write
    OPEN_MODE_WB = 5, /// write binary
    OPEN_MODE_WP = 6, /// write plus
    OPEN_MODE_WPB = 7, /// write plus binary
    OPEN_MODE_A = 8, /// append
    OPEN_MODE_AB = 9, /// append binary
    OPEN_MODE_AP = 10, /// append plus
    OPEN_MODE_APB = 11, /// append plus binary
} open_mode_t;

typedef enum {
    //
    // hardware exceptions
    //
    ADP_Stopped_BranchThroughZero = 0x20000,
    ADP_Stopped_UndefinedInstr = 0x20001,
    ADP_Stopped_SoftwareInterrupt = 0x20002,
    ADP_Stopped_PrefetchAbort = 0x20003,
    ADP_Stopped_DataAbort = 0x20004,
    ADP_Stopped_AddressException = 0x20005,
    ADP_Stopped_IRQ = 0x20006,
    ADP_Stopped_FIQ = 0x20007,
    //
    // software exceptions
    //
    ADP_Stopped_BreakPoint = 0x20020,
    ADP_Stopped_WatchPoint = 0x20021,
    ADP_Stopped_StepComplete = 0x20022,
    ADP_Stopped_RunTimeErrorUnknown = 0x20023,
    ADP_Stopped_InternalError = 0x20024,
    ADP_Stopped_UserInterruption = 0x20025,
    ADP_Stopped_ApplicationExit = 0x20026,
    ADP_Stopped_StackOverflow = 0x20027,
    ADP_Stopped_DivisionByZero = 0x20028,
    ADP_Stopped_OSSpecific = 0x20029,
} exit_reason_t;

typedef struct heapinfo_block {
    int32_t heap_base;
    int32_t heap_limit;
    int32_t stack_base;
    int32_t stack_limit;
} heapinfo_block_t;

int32_t sys_clock();
int32_t sys_close(int32_t handle);
int32_t sys_elapsed(uint64_t *);
int32_t sys_errno();
void sys_exit(int32_t reason);
void sys_exit(uint32_t reason, int32_t code);
int32_t sys_flen(int32_t handle);
int32_t sys_getcmdline(void *buf, uint32_t size);
int32_t sys_heapinfo(heapinfo_block_t *block);
int32_t sys_iserror(int32_t status);
int32_t sys_istty(int32_t handle);
int32_t sys_open(const char *path, open_mode_t mode, uint32_t path_len);
int32_t sys_read(int32_t handle, void *buf, uint32_t count);
int32_t sys_readc();
int32_t sys_remove(char *path, uint32_t path_len);
int32_t sys_rename(char *old_path, uint32_t old_path_len, char *new_path, uint32_t new_path_len);
int32_t sys_seek(int32_t handle, int32_t pos);
int32_t sys_system(const char *command, uint32_t length);
int32_t sys_tickfreq();
int32_t sys_time();
int32_t sys_tmpnam(void *buf, int32_t target_id, uint32_t buf_size);
int32_t sys_write(int32_t handle, void *buf, uint32_t count);
void sys_writec(char ch);
void sys_write0(char *str);

} // namespace semihosting
