#pragma once

#include <cstdlib>

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus
/**
 * \brief Get the long file name of a given file.
 *
 * Given a full path to a file, this extracts the long file name (the basename
 * only, not the path) and stores it to the provided buffer (the lfn_size is
 * the size of the buffer, and must accomodate space for '\0').
 *
 * Note that the path is _not_ constant and it is temporarily modified in-place
 * (and fixed) later on.
 *
 * This is a "best effort" approach. It never fails, but if there's a failure
 * internally, the short file name from the path might be returned.
 */
void get_LFN(char *lfn, size_t lfn_size, char *path);

/**
 * \brief Get a SFN path for a given path.
 *
 * Given a full path to a file (either LFN or SFN, doesn't matter), converts
 * the last part to SFN, in-place. This assumes LFN is never shorter than SFN,
 * therefore the result can't overflow the buffer.
 *
 * This might fail internally (eg. for files that don't exist), in such case
 * the path is not modified.
 */
void get_SFN_path(char *path);

#ifdef __cplusplus
}
#endif
