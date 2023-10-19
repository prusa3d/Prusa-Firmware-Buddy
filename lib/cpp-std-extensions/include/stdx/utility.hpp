#pragma once

#include <stdx/compiler.hpp>

#include <cstddef>

namespace stdx {
inline namespace v1 {

template <typename... Fs> struct overload : Fs... {
    using Fs::operator()...;
};

#if __cpp_deduction_guides < 201907L
template <typename... Fs> overload(Fs...) -> overload<Fs...>;
#endif

namespace literals {
CONSTEVAL auto operator""_b(char const *, std::size_t) -> bool { return true; }
CONSTEVAL auto operator""_true(char const *, std::size_t) -> bool {
    return true;
}
CONSTEVAL auto operator""_false(char const *, std::size_t) -> bool {
    return false;
}

// NOLINTBEGIN(google-runtime-int)
CONSTEVAL auto operator""_k(unsigned long long int n)
    -> unsigned long long int {
    return n * 1'000u;
}
CONSTEVAL auto operator""_M(unsigned long long int n)
    -> unsigned long long int {
    return n * 1'000'000u;
}
CONSTEVAL auto operator""_G(unsigned long long int n)
    -> unsigned long long int {
    return n * 1'000'000'000u;
}

CONSTEVAL auto operator""_ki(unsigned long long int n)
    -> unsigned long long int {
    return n * 1'024u;
}
CONSTEVAL auto operator""_Mi(unsigned long long int n)
    -> unsigned long long int {
    return n * 1'024ull * 1'024ull;
}
CONSTEVAL auto operator""_Gi(unsigned long long int n)
    -> unsigned long long int {
    return n * 1'024ull * 1'024ull * 1'024ull;
}
// NOLINTEND(google-runtime-int)
} // namespace literals

[[noreturn]] inline auto unreachable() -> void { __builtin_unreachable(); }

} // namespace v1
} // namespace stdx
