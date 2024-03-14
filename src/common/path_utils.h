#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Deduplicates successive slashes from a path, in-place.
void dedup_slashes(char *filename);

#ifdef __cplusplus
}
#endif
