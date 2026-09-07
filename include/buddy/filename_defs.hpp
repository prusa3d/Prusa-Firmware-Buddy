/// @file
/// Single source of truth for filename and path length limits.
#pragma once

// Has to be a macro as it's used in FatFS's ffconf.h.
// C++ code should use filename_defs::max_filename_length below.
#define MAX_FILENAME_LENGTH 167

// NAME_MAX is normally provided by the system; fallback for hosts/tests.
#ifndef NAME_MAX
    #define NAME_MAX 255
#endif

#ifdef __cplusplus

    #include <cstddef>

namespace filename_defs {

/// Longest display file name (basename), e.g. "part.bgcode".
inline constexpr size_t max_filename_length = MAX_FILENAME_LENGTH;
inline constexpr size_t filename_buffer_size = max_filename_length + 1;

/// Longest canonical path, including the mount point. The path is built from
/// short segments (SFN on FAT), so long display names don't grow it.
inline constexpr size_t max_path_length = 103;
inline constexpr size_t path_buffer_size = max_path_length + 1;

} // namespace filename_defs

#endif // __cplusplus
