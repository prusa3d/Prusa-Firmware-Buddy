#include "percent_encode.h"

#include <cstdint>

namespace http {

namespace {

    bool is_unreserved(uint8_t byte, std::string_view extra_unreserved) {
        // Not the ctype functions, those are locale dependent.
        return (byte >= 'a' && byte <= 'z') || (byte >= 'A' && byte <= 'Z') || (byte >= '0' && byte <= '9')
            || extra_unreserved.find(static_cast<char>(byte)) != std::string_view::npos;
    }

} // namespace

size_t percent_encode(std::string_view input, std::string_view extra_unreserved, char *buffer, size_t buffer_len) {
    static const char hex[] = "0123456789ABCDEF";
    size_t pos = 0;

    for (const char c : input) {
        const uint8_t byte = c;
        if (is_unreserved(byte, extra_unreserved)) {
            if (pos + 1 > buffer_len) {
                break;
            }
            buffer[pos++] = c;
        } else {
            if (pos + 3 > buffer_len) {
                break;
            }
            buffer[pos++] = '%';
            buffer[pos++] = hex[byte >> 4];
            buffer[pos++] = hex[byte & 0xf];
        }
    }

    return pos;
}

} // namespace http
