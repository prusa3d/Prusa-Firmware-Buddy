#include "bsod.h"
#include "safe_state.h"
#include <common/sys.hpp>

extern "C" [[gnu::noinline]] void abort() {
    const auto *callee_address = __builtin_extract_return_addr(__builtin_return_address(0));
    _bsod("aborted %p", nullptr, -1, callee_address);
}

void __assert_func(const char *file_path, int line, const char * /*func*/, const char *msg) {
    const char *slash_idx = std::strrchr(file_path, '/');
    const char *file_name = slash_idx != nullptr ? slash_idx + 1 : file_path;
    _bsod("ASSERT %s", file_name, line, msg);
}

extern "C" int _isatty(int __attribute__((unused)) fd) {
    // TTYs are not supported
    return 0;
}
