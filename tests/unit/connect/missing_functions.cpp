#include <cstdint>
#include <cstdlib>
#include <cstring>

extern "C" {

size_t strlcpy(char *, const char *, size_t);

uint32_t ticks_ms() {
    // Mock only
    return 0;
}

void get_LFN(char *lfn, size_t lfn_size, char *path) {
    strlcpy(lfn, basename(path), lfn_size);
}
}
