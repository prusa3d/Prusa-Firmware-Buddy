#pragma once
#include <string.h>
#include <sys/types.h>
#include <dirent.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief Extract the base name of a filename.
 *
 * Note: The name is not "basename" due to collisions somewhere around C/C++
 * linkage in tests :-|
 *
 * \return The part after the last slash, or the original string if there's no
 *   slash. It points into the original (doesn't copy anything).
 */
const char *basename_b(const char *path);

/**
 * \brief Extract the parent dir name of a filename
 *
 * Modifies the path buffer, so that it is the parent path of the original.
 * In practise it means it just replaces the last slash with '\0'.
 *
 * In case no slash is present it leaves the buffer unmodified.
 *
 */
void dirname(char *path);

/**
 * Extract the LFN of the given dirent.
 *
 * Falls back to SFN / normal file name in case LFN is not available or on an
 * „real OS“ for tests where such thing doesn't exist.
 */
const char *dirent_lfn(const struct dirent *ent);

#ifdef __cplusplus
}
#endif
