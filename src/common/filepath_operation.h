#include <string.h>

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

#ifdef __cplusplus
}
#endif
