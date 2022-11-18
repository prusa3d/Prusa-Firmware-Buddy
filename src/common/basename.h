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

#ifdef __cplusplus
}
#endif
