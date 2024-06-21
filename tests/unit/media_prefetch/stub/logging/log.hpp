// Unittest stub

#pragma once

#include <cstdarg>
#include <cstdio>
#include <array>

#include <catch2/catch.hpp>

auto log(const char *fmt, ...) {
    std::array<char, 256> buffer;
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer.data(), buffer.size(), fmt, args);
    va_end(args);

    return buffer;
}

#define LOG_COMPONENT_DEF(...)

#define log_debug(component, ...)   INFO(log(__VA_ARGS__).data())
#define log_warning(component, ...) INFO(log(__VA_ARGS__).data())
#define log_info(component, ...)    INFO(log(__VA_ARGS__).data())

#define bsod(err) FAIL(err)
