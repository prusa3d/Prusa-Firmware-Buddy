/// @file
#pragma once

#include "forward.hpp"
#include "inplace_promise.hpp"

#include <coroutine>
#include <cstdlib>
#include <optional>
#include <utility>

namespace coroutines {

template <typename Tag, std::size_t allocated_size, typename ReturnType = void>
class InplaceCoroutine {
public:
    using promise_type = InplacePromise<Tag, allocated_size, ReturnType>;
    using handle_type = std::coroutine_handle<InplacePromise<Tag, allocated_size, ReturnType>>;
    using functor_return_type = std::conditional_t<std::is_same_v<ReturnType, void>, bool, std::optional<ReturnType>>;
    InplaceCoroutine(handle_type h)
        : handle(h) {}
    ~InplaceCoroutine() {
        if (handle) {
            handle.destroy();
        }
    }

    InplaceCoroutine(const InplaceCoroutine<Tag, allocated_size, ReturnType> &) = delete;
    auto &operator=(const InplaceCoroutine<Tag, allocated_size, ReturnType> &) = delete;
    constexpr InplaceCoroutine(InplaceCoroutine<Tag, allocated_size, ReturnType> &&other)
        : handle(std::exchange(other.handle, {})) {}
    constexpr auto &operator=(InplaceCoroutine<Tag, allocated_size, ReturnType> &&other) {
        if (this == &other) {
            return *this;
        }
        if (handle) {
            handle.destroy();
        }
        handle = std::exchange(other.handle, {});
        return *this;
    };

    functor_return_type operator()() {
        const auto done_return_value = [&]() -> functor_return_type {
            if constexpr (std::is_same_v<void, ReturnType>) {
                return true;
            } else {
                return handle.promise().value;
            }
        };
        if (handle.done()) {
            return done_return_value();
        }
        handle.resume();
        if (handle.done()) {
            return done_return_value();
        }
        if constexpr (std::is_same_v<void, ReturnType>) {
            return false;
        } else {
            return std::nullopt;
        }
    }

protected:
    handle_type handle;
};

} // namespace coroutines
