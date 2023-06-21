#include "os_porting.hpp"

#if defined(__unix__) || defined(__APPLE__) || defined(__WIN32__)
    #include "string.h"

uint32_t ticks_ms(void) {

    struct timeval tv;

    // Get current time
    gettimeofday(&tv, NULL);

    // Convert resulting value to milliseconds
    return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

uint32_t osDelay(uint32_t millisec) {
    std::this_thread::sleep_for(std::chrono::milliseconds(millisec));
    return 0;
}

void osThreadTerminate(uint32_t) {
    throw std::exception();
}

osThreadId osThreadGetId() {
    return 1;
}

size_t strlcpy(char *dst, const char *src, size_t maxlen) {
    const size_t srclen = strlen(src);
    if (srclen + 1 < maxlen) {
        memcpy(dst, src, srclen + 1);
    } else if (maxlen != 0) {
        memcpy(dst, src, maxlen - 1);
        dst[maxlen - 1] = '\0';
    }
    return srclen;
}

#endif
