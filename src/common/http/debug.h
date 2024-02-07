#pragma once
#if defined(__unix__) || defined(__APPLE__) || defined(__WIN32__)
    #include <stdio.h>
    #define log_debug(x, ...) printf(__VA_ARGS__)
    #define log_error(x, ...) printf(__VA_ARGS__)
    #define LOG_COMPONENT_DEF(...)
    #define LOG_COMPONENT_REF(...)
#else
    #include "log.h"
#endif
