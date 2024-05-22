#pragma once

#include <stdx/ct_string.hpp>

#include <utility>

namespace stdx {
inline namespace v1 {
struct default_panic_handler {
    template <typename... Args>
    static auto panic(Args &&...) noexcept -> void {}

#if __cplusplus >= 202002L
    template <ct_string, typename... Args>
    static auto panic(Args &&...) noexcept -> void {}
#endif
};

template <typename...> inline auto panic_handler = default_panic_handler{};

template <typename... Ts, typename... Args> auto panic(Args &&...args) -> void {
    panic_handler<Ts...>.panic(std::forward<Args>(args)...);
}

#if __cplusplus >= 202002L
template <ct_string S, typename... Ts, typename... Args>
auto panic(Args &&...args) -> void {
    panic_handler<Ts...>.template panic<S>(std::forward<Args>(args)...);
}
#endif
} // namespace v1
} // namespace stdx

#if __cplusplus >= 202002L
#define STDX_PANIC(MSG, ...)                                                   \
    [] {                                                                       \
        using stdx::ct_string_literals::operator""_cts;                        \
        stdx::panic<MSG##_cts>(__VA_ARGS__);                                   \
    }()
#else
#define STDX_PANIC(...) stdx::panic(__VA_ARGS__)
#endif
