#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Deduplicates successive slashes from a path, in-place.
void dedup_slashes(char *filename);

[[nodiscard]] bool file_exists(const char *path);

#ifdef __cplusplus
}
#endif
