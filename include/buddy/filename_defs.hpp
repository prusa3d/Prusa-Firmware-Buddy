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

    #include <option/has_internal_storage_flash.h>

namespace filename_defs {

/// Longest display file name (basename), e.g. "part.bgcode".
inline constexpr size_t max_filename_length = MAX_FILENAME_LENGTH;
inline constexpr size_t filename_buffer_size = max_filename_length + 1;

/// Longest short (SFN) basename. FAT's 8.3 name can bloat when pseudo-encoded
/// to UTF-8; single-name filesystems have no separate short name.
inline constexpr size_t max_short_filename_length = 24;
inline constexpr size_t short_filename_buffer_size = max_short_filename_length + 1;

/// Longest canonical path on FAT, built from short (SFN) components - long
/// display names don't grow it.
inline constexpr size_t max_sfn_path_length = 103;

    /// Longest canonical path, including the mount point.
    #if HAS_INTERNAL_STORAGE_FLASH()
// Internal (littlefs) storage: a path component is a full-length name, so the
// budget must hold the mount point + a filename + (shallow) nesting.
inline constexpr size_t max_path_length = 255;
    #else
inline constexpr size_t max_path_length = max_sfn_path_length;
    #endif
inline constexpr size_t path_buffer_size = max_path_length + 1;

/// Longest path with an LFN last segment (SFN dir prefix + LFN basename), as
/// produced when creating an entity that has no SFN yet. Upper bound. On
/// single-name filesystems (littlefs) there is no extra LFN segment; the
/// canonical budget fits within this bound (checked below).
inline constexpr size_t max_hybrid_path_length = max_sfn_path_length + max_filename_length;
inline constexpr size_t hybrid_path_buffer_size = max_hybrid_path_length + 1;
static_assert(max_hybrid_path_length >= max_path_length);

} // namespace filename_defs

#endif // __cplusplus
