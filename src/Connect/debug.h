#pragma once
#if defined(__unix__) || defined(__APPLE__)
    #include <stdio.h>
    #define CONNECT_DEBUG     printf
    #define log_debug(x, ...) printf(__VA_ARGS__)
    #define LOG_COMPONENT_DEF(...)
#else
    #include "log.h"
    #define CONNECT_DEBUG(...)
#endif
