#pragma once

#include <charconv>
#include <string_view>

namespace http {
bool url_decode(std::string_view url, char *decoded_url, size_t decoded_url_len);
}
