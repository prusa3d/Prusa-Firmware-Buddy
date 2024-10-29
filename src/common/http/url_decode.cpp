#include "url_decode.h"
#include <str_utils.hpp>

namespace http {
bool url_decode(std::string_view url, char *decoded_url, size_t decoded_url_len) {
    if (decoded_url_len == 0) {
        return false;
    }

    size_t out_index = 0;
    for (size_t i = 0; i < url.size(); i++) {
        switch (url[i]) {
        case '+':
            decoded_url[out_index] = ' ';
            break;
        case '%': {
            if (decoded_url_len < i + 3) {
                return false;
            }
            int ascii_value;
            auto curr_iter = url.begin() + i;
            auto result = from_chars_light(curr_iter + 1, curr_iter + 3, ascii_value, 16);
            if (result.ec != std::errc {}) {
                return false;
            }

            decoded_url[out_index] = static_cast<char>(ascii_value);
            i = i + 2;
            break;
        }
        default:
            decoded_url[out_index] = url[i];
            break;
        }
        if (++out_index >= decoded_url_len) {
            return false;
        }
    }
    decoded_url[out_index] = '\0';
    return true;
}
} // namespace http
