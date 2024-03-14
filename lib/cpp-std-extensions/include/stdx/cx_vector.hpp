#pragma once

#include <stdx/concepts.hpp>

#include <array>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>

namespace stdx {
inline namespace v1 {
template <typename T, std::size_t N> class cx_vector {
    std::array<T, N> storage{};
    std::size_t current_size{};

  public:
    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = value_type &;
    using const_reference = value_type const &;
    using pointer = value_type *;
    using const_pointer = value_type const *;
    using iterator = pointer;
    using const_iterator = const_pointer;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    constexpr cx_vector() = default;
    template <typename... Ts,
              std::enable_if_t<((sizeof...(Ts) <= N) and ... and
                                stdx::convertible_to<value_type, Ts>),
                               int> = 0>
    constexpr explicit cx_vector(Ts const &...ts)
        : storage{static_cast<value_type>(ts)...}, current_size{sizeof...(Ts)} {
    }

    [[nodiscard]] constexpr auto begin() -> iterator {
        return std::begin(storage);
    }
    [[nodiscard]] constexpr auto begin() const -> const_iterator {
        return std::begin(storage);
    }
    [[nodiscard]] constexpr auto cbegin() const -> const_iterator {
        return std::cbegin(storage);
    }

    [[nodiscard]] constexpr auto end() -> iterator {
        return std::begin(storage) + current_size;
    }
    [[nodiscard]] constexpr auto end() const -> const_iterator {
        return std::begin(storage) + current_size;
    }
    [[nodiscard]] constexpr auto cend() const -> const_iterator {
        return std::cbegin(storage) + current_size;
    }

    [[nodiscard]] constexpr auto rbegin() -> reverse_iterator { return end(); }
    [[nodiscard]] constexpr auto rbegin() const -> const_reverse_iterator {
        return end();
    }
    [[nodiscard]] constexpr auto crbegin() const -> const_reverse_iterator {
        return cend();
    }

    [[nodiscard]] constexpr auto rend() -> reverse_iterator { return begin(); }
    [[nodiscard]] constexpr auto rend() const -> const_reverse_iterator {
        return begin();
    }
    [[nodiscard]] constexpr auto crend() const -> const_reverse_iterator {
        return cbegin();
    }

    [[nodiscard]] constexpr auto front() -> reference { return storage[0]; }
    [[nodiscard]] constexpr auto front() const -> const_reference {
        return storage[0];
    }
    [[nodiscard]] constexpr auto back() -> reference {
        return storage[current_size - 1];
    }
    [[nodiscard]] constexpr auto back() const -> const_reference {
        return storage[current_size - 1];
    }

    [[nodiscard]] constexpr auto size() const -> size_type {
        return current_size;
    }
    constexpr static std::integral_constant<size_type, N> capacity{};

    [[nodiscard]] constexpr auto operator[](std::size_t index) -> reference {
        return storage[index];
    }
    [[nodiscard]] constexpr auto operator[](std::size_t index) const
        -> const_reference {
        return storage[index];
    }

    template <std::size_t Index>
    [[nodiscard]] constexpr auto get() -> reference {
        return std::get<Index>(storage);
    }
    template <std::size_t Index>
    [[nodiscard]] constexpr auto get() const -> const_reference {
        return std::get<Index>(storage);
    }

    [[nodiscard]] constexpr auto full() const -> bool {
        return current_size == N;
    }
    [[nodiscard]] constexpr auto empty() const -> bool {
        return current_size == 0u;
    }

    constexpr auto clear() -> void { current_size = 0; }

    constexpr auto push_back(value_type const &value) -> reference {
        return storage[current_size++] = value;
    }
    constexpr auto push_back(value_type &&value) -> reference {
        return storage[current_size++] = std::move(value);
    }

    [[nodiscard]] constexpr auto pop_back() -> value_type {
        return storage[--current_size];
    }

  private:
    template <typename F>
    friend constexpr auto resize_and_overwrite(cx_vector &v, F &&f) -> void {
        v.current_size =
            std::forward<F>(f)(std::data(v.storage), std::size(v.storage));
    }

    [[nodiscard]] friend constexpr auto operator==(cx_vector const &lhs,
                                                   cx_vector const &rhs)
        -> bool {
        if (lhs.size() != rhs.size()) {
            return false;
        }
        for (auto i = size_type{}; i < lhs.size(); ++i) {
            if (lhs[i] != rhs[i]) {
                return false;
            }
        }
        return true;
    }

#if __cpp_impl_three_way_comparison < 201907L
    [[nodiscard]] friend constexpr auto operator!=(cx_vector const &lhs,
                                                   cx_vector const &rhs)
        -> bool {
        return not(lhs == rhs);
    }
#endif
};

template <typename T, typename... Ts>
cx_vector(T, Ts...) -> cx_vector<T, 1 + sizeof...(Ts)>;

template <std::size_t I, typename T, std::size_t N>
auto get(cx_vector<T, N> &v) -> decltype(auto) {
    return v.template get<I>();
}

template <std::size_t I, typename T, std::size_t N>
auto get(cx_vector<T, N> const &v) -> decltype(auto) {
    return v.template get<I>();
}
} // namespace v1
} // namespace stdx
