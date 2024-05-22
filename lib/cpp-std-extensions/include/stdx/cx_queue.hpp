#pragma once

#include <stdx/panic.hpp>

#include <array>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>

namespace stdx {
inline namespace v1 {
struct unsafe_overflow_policy {
    template <typename... Args> constexpr static auto check_push(Args &&...) {}
    template <typename... Args> constexpr static auto check_pop(Args &&...) {}
};

struct safe_overflow_policy {
    constexpr static auto check_push(std::size_t size, std::size_t capacity) {
        if (size >= capacity) {
            STDX_PANIC("cx_queue overflow!");
        }
    }
    constexpr static auto check_pop(std::size_t size) {
        if (size <= 0) {
            STDX_PANIC("cx_queue underflow!");
        }
    }
};

template <typename T, std::size_t N,
          typename OverflowPolicy = safe_overflow_policy>
class cx_queue {
    std::array<T, N> storage{};
    std::size_t push_index{N - 1};
    std::size_t pop_index{};
    std::size_t current_size{};

  public:
    using value_type = T;
    using size_type = std::size_t;
    using reference = value_type &;
    using const_reference = value_type const &;

    [[nodiscard]] constexpr auto size() const -> size_type {
        return current_size;
    }
    constexpr static std::integral_constant<size_type, N> capacity{};

    [[nodiscard]] constexpr auto full() const -> bool {
        return current_size == N;
    }
    [[nodiscard]] constexpr auto empty() const -> bool {
        return current_size == 0u;
    }

    constexpr auto clear() -> void {
        pop_index = 0;
        push_index = N - 1;
        current_size = 0;
    }

    [[nodiscard]] constexpr auto front() & -> reference {
        OverflowPolicy::check_pop(current_size);
        return storage[pop_index];
    }
    [[nodiscard]] constexpr auto front() const & -> const_reference {
        OverflowPolicy::check_pop(current_size);
        return storage[pop_index];
    }
    [[nodiscard]] constexpr auto back() & -> reference {
        OverflowPolicy::check_pop(current_size);
        return storage[push_index];
    }
    [[nodiscard]] constexpr auto back() const & -> const_reference {
        OverflowPolicy::check_pop(current_size);
        return storage[push_index];
    }

    constexpr auto push(value_type const &value) -> reference {
        OverflowPolicy::check_push(current_size, N);
        if (++push_index == N) {
            push_index = 0;
        }
        ++current_size;
        return storage[push_index] = value;
    }
    constexpr auto push(value_type &&value) -> reference {
        OverflowPolicy::check_push(current_size, N);
        if (++push_index == N) {
            push_index = 0;
        }
        ++current_size;
        return storage[push_index] = std::move(value);
    }

    [[nodiscard]] constexpr auto pop() -> value_type {
        OverflowPolicy::check_pop(current_size);
        auto entry = std::move(storage[pop_index++]);
        if (pop_index == N) {
            pop_index = 0;
        }
        --current_size;
        return entry;
    }
};
} // namespace v1
} // namespace stdx
