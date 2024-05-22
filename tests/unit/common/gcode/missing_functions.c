#include <string.h>

size_t strlcat(char *dst, const char *src, size_t size) {
    /*
     * Note: this is not _completely_ correct. Specifically, if dst is longer
     * than size, it does bad things. Good enough for test purposes.
     */
    const size_t start = strlen(dst);
    strncat(dst + start, src, size - 1 - start);
    return start + strlen(src);
}

size_t
strlcpy(char *dst, const char *src, size_t siz) {
    char *d = dst;
    const char *s = src;
    size_t n = siz;
    /* Copy as many bytes as will fit */
    if (n != 0) {
        while (--n != 0) {
            if ((*d++ = *s++) == '\0') {
                break;
            }
        }
    }
    /* Not enough room in dst, add NUL and traverse rest of src */
    if (n == 0) {
        if (siz != 0) {
            *d = '\0'; /* NUL-terminate dst */
        }
        while (*s++)
            ;
    }
    return (s - src - 1); /* count does not include NUL */
}
