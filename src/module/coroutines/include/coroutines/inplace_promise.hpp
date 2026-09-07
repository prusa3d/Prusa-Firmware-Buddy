/// @file
#pragma once

#include "forward.hpp"

#include <array>
#include <concepts>
#include <coroutine>
#include <cstddef>
#include <cstdlib>
#include <optional>

namespace coroutines {
template <typename Tag, std::size_t allocated_size, typename ReturnType>
struct InplacePromiseBase {
    constexpr std::suspend_always initial_suspend() noexcept { return {}; }
    constexpr std::suspend_always final_suspend() noexcept { return {}; }
    inline void unhandled_exception() {
        std::abort();
    }

    static inline InplaceCoroutine<Tag, allocated_size, ReturnType> get_return_object_on_allocation_failure() {
        std::abort();
    }

    static constexpr void *operator new(std::size_t n) noexcept {
        if (n > allocated_size) {
            std::abort();
        }
        data.fill(std::byte { 0 });
        return data.data();
    }

    static constexpr void operator delete(void *) noexcept {}

protected:
    static std::array<std::byte, allocated_size> data alignas(std::max_align_t);
};
template <typename Tag, std::size_t allocated_size, typename ReturnType>
std::array<std::byte, allocated_size> InplacePromiseBase<Tag, allocated_size, ReturnType>::data;

template <typename Tag, std::size_t allocated_size, typename ReturnType>
struct InplacePromise : public InplacePromiseBase<Tag, allocated_size, ReturnType> {
    static constexpr bool has_deducing_this =
#if defined(__cpp_explicit_this_parameter)
        true;
#else
        false;
#endif
    static_assert(!has_deducing_this, "FIXME: Move get_return_object to inplace_promise_base when we have deducing this support in compiler");
    constexpr std::coroutine_handle<InplacePromise<Tag, allocated_size, ReturnType>> get_return_object() { return std::coroutine_handle<InplacePromise<Tag, allocated_size, ReturnType>>::from_promise(*this); }
    std::optional<ReturnType> value = std::nullopt;
    template <std::convertible_to<ReturnType> From>
    void return_value(From &&from) {
        value = std::move(from);
    }
};

template <typename Tag, std::size_t allocated_size>
struct InplacePromise<Tag, allocated_size, void> : public InplacePromiseBase<Tag, allocated_size, void> {
    constexpr std::coroutine_handle<InplacePromise<Tag, allocated_size, void>> get_return_object() { return std::coroutine_handle<InplacePromise<Tag, allocated_size, void>>::from_promise(*this); }
    void return_void() {}
};
} // namespace coroutines
