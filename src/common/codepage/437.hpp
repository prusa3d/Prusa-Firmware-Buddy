#pragma once

#include <cstdlib>
#include <cstdint>

namespace codepage {

// Convert from cp437 to utf8.
//
// As each input byte has a corresponding unicode codepoint, this conversion is
// infallible.
//
// The input and output buffers must not overlap.
//
// It may write up to input_size * 3 bytes to the output buffer, returns number
// of bytes written.
//
// The input doesn't have to be null-terminated and it doesn't stop at
// intermediate null bytes (eg. they are encoded into the output and it
// continues to the whole size). Similarly, output is _not_ null terminated.
size_t cp437_to_utf8(uint8_t *output, const uint8_t *input, size_t input_size);

} // namespace codepage
