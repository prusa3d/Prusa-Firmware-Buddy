/// \file
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

[[noreturn]] void __attribute__((noreturn, format(__printf__, 1, 4)))
_bsod(const char *fmt, const char *file_name, int line_number, ...);

#define bsod(fmt, ...) _bsod(fmt, __FILE_NAME__, __LINE__, ##__VA_ARGS__)

/// Declare codepath as unreachable.
///
/// It is a hard error if the codepath is actually taken.
/// This is _not_ std::unreachable() and that is correct.
/// We want the check to be present even in release mode.
///
/// This is a macro. We want to provide some basic debug
/// information in order to distinguisgh BSODs at first
/// glance, without needing to produce full stack trace.
/// Also, we want the different callsites to actually
/// differ because inter-procedural optimization might
/// fold them and prevent setting breakpoints.
#define bsod_unreachable() bsod("unreachable")

/// Convenience bsod() macro for system errors
// Every HAL_init fail does not necessarily need a custom message
#define bsod_system() bsod("system error")

#ifdef __cplusplus
    #define CHECK_SYNTAX_ASSERT(expr) static_assert(requires { static_cast<bool>(expr); })
#else
    #define CHECK_SYNTAX_ASSERT(expr) _Static_assert(sizeof(!(expr)), #expr)
#endif

#define ASSERT_IMPL(expr)         \
    if (!(expr)) {                \
        bsod("ASSERT %s", #expr); \
    }                             \
    __attribute__((assume(expr)))

#ifdef NDEBUG
    #define debug_assert(expr) CHECK_SYNTAX_ASSERT(expr)
#else
    #define debug_assert(expr) ASSERT_IMPL(expr)
#endif

#define release_assert(expr) ASSERT_IMPL(expr)

#if defined(__cpp_contracts) && __cpp_contracts >= 202502L
// contract_assert is a language keyword
#else
    #define contract_assert(expr)                                 \
        if (!(expr)) {                                            \
            _bsod("CONTRACT %s", __FILE_NAME__, __LINE__, #expr); \
        }                                                         \
        __attribute__((assume(expr)))
#endif

#ifdef __cplusplus
}
#endif
