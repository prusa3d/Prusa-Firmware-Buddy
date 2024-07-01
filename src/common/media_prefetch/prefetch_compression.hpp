#pragma once

#include <span>
#include <cstdint>
#include <optional>

namespace media_prefetch {

/// Inplace compacts the provided gcode - strips comments, leading whitespaces
/// and for Gx gcodes, spaces between parameters
/// \returns strlen of the compacted gcode
size_t compact_gcode(char *inplace_buffer);

} // namespace media_prefetch
