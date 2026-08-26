#pragma once

#include <cstddef>
#include <string_view>

namespace http {

/// Percent-encodes input into buffer, escaping everything but alphanumerics and
/// the bytes listed in extra_unreserved (what may stay depends on the context,
/// see eg. attr-char of RFC 5987).
///
/// The result is not null-terminated and gets truncated if it doesn't fit,
/// never in the middle of an escape.
///
/// \returns The number of bytes written.
size_t percent_encode(std::string_view input, std::string_view extra_unreserved, char *buffer, size_t buffer_len);

} // namespace http
