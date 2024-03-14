#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <transfers/transfer_file_check.hpp>

namespace transfers {
bool is_valid_transfer(const MutablePath &) {
    return false;
}
} // namespace transfers

extern "C" {

size_t strlcat(char *dst, const char *src, size_t size) {
    /*
     * Note: this is not _completely_ correct. Specifically, if dst is longer
     * than size, it does bad things. Good enough for test purposes.
     */
    const size_t start = strlen(dst);
    strncat(dst + start, src, size - 1 - start);
    return start + strlen(src);
}
}
