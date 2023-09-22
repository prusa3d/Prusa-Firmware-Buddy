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

/// Convert from utf8 to cp437.
///
/// The conversion happens in-place (as the output is at most the size of the
/// input, it is possible). Returns the new size.
///
/// Null bytes are not considered a terminator and are just "decoded".
///
/// The function does not expect invalid input, it is expected the use case is
/// data that were previously encoded by cp437_to_utf8 (passed through connect,
/// for example). If it happens nonetheless, the "unconvertible" bytes are
/// passed through. For that reason, be careful of the context in which the
/// function is used ‒ in particular, multiple different inputs may produce the
/// same output.
size_t utf8_to_cp437(uint8_t *inout, size_t input_size);

} // namespace codepage
