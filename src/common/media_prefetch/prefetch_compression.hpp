#pragma once

#include <span>
#include <cstdint>
#include <optional>

namespace media_prefetch {

/// Inplace compacts the provided gcode - strips comments, leading whitespaces
/// and for Gx gcodes, spaces between parameters
/// \returns strlen of the compacted gcode
size_t compact_gcode(char *inplace_buffer);

/// Compresses a single gcode command using a meatpack-like algorithm.
/// This should have ~50% compression rate for most gcodes.
/// \returns the compressed data length or \p nullopt on failure (not all strings are compressable).
/// !!! The compressed data is data, not null terminated
std::optional<size_t> compress_gcode(const char *input, std::span<uint8_t> output);

/// Decompresses a single gcode command previously compressed with \p compress_gcode
/// \param compressed_len must match what \p compress_gcode returned.
/// Providing invalid data is UB.
void decompress_gcode(const uint8_t *input, int compressed_len, std::span<char> output);

} // namespace media_prefetch
