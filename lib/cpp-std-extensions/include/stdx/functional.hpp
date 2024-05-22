#pragma once

#include <type_traits>

namespace stdx {
inline namespace v1 {

template <typename F> struct with_result_of : F {
    using R = std::invoke_result_t<F>;
    // NOLINTNEXTLINE(google-explicit-constructor)
    constexpr operator R() const
        noexcept(noexcept(static_cast<F const &>(*this)())) {
        return static_cast<F const &>(*this)();
    }
    // NOLINTNEXTLINE(google-explicit-constructor)
    constexpr operator R() noexcept(noexcept(static_cast<F &>(*this)())) {
        return static_cast<F &>(*this)();
    }
};

#if __cpp_deduction_guides < 201907L
template <typename F> with_result_of(F) -> with_result_of<F>;
#endif

} // namespace v1
} // namespace stdx
