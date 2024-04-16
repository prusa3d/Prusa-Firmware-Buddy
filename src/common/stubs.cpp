#include "bsod.h"
#include "safe_state.h"

void abort() {
    bsod("aborted");
}

void __assert_func(const char *file, int line, const char * /*func*/, const char *msg) {
#if _DEBUG
    if (CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk) {
        buddy_disable_heaters(); // put HW to safe state
        __asm("BKPT #0\n"); /* Only halt mcu if debugger is attached */
    } else
#endif
        _bsod("ASSERT %s", file, line, msg);
}

extern "C" int _isatty(int __attribute__((unused)) fd) {
    // TTYs are not supported
    return 0;
}
