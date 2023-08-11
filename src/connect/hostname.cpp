#include "hostname.hpp"

#include <cstring>
#include <cstdint>

namespace connect_client {

namespace {

    const constexpr char *suffixes[] = {
        "connect.prusa3d.com",
        "prusa3d.com",
    };

    constexpr uint8_t compress_mark = 1;
} // namespace

bool compress_host(const char *host, char *buffer, size_t buffer_len) {
    size_t len = strlen(host);
    for (size_t i = 0; i < (sizeof suffixes / sizeof *suffixes); i++) {
        size_t suf_len = strlen(suffixes[i]);

        if (suf_len > len) {
            // Hostname shorter than suffix - suffix is for sure not there.
            continue;
        }

        if (len - suf_len + 2 >= buffer_len) {
            // The result would be longer even if it was shortened by the suffix, so no need to check.
            continue;
        }

        if (strcasecmp(suffixes[i], host + len - suf_len) == 0) {
            size_t l_prefix = len - suf_len;
            memcpy(buffer, host, l_prefix);
            buffer[l_prefix] = compress_mark;
            // Avoid 0 index for strlen to work on that string.
            buffer[l_prefix + 1] = i + 1;
            buffer[l_prefix + 2] = '\0';
            return true;
        }
    }

    if (len >= buffer_len) {
        return false;
    }

    // Just checked the size, strcpy is fine
    strcpy(buffer, host);
    return true;
}

void decompress_host(char *host, size_t host_buffer) {
    size_t len = strlen(host);
    if (len < 2) {
        return;
    }

    if (host[len - 2] != compress_mark) {
        return;
    }

    // Re-adjust back to 0-based indices
    size_t idx = host[len - 1] - 1;

    if (idx >= (sizeof suffixes / sizeof *suffixes)) {
        // idx out of range
        return;
    }

    size_t suffix_len = strlen(suffixes[idx]);
    size_t new_len = len - 2 + suffix_len;

    if (new_len + 1 > host_buffer) {
        // The result wouldn't fit, so don't expand.
        // (That shouldn't happen)
        return;
    }

    strcpy(host + len - 2, suffixes[idx]);
}

} // namespace connect_client
