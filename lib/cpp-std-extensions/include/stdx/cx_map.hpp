#pragma once

#include <stdx/concepts.hpp>
#include <stdx/utility.hpp>

#include <array>
#include <cstddef>
#include <iterator>
#include <type_traits>

namespace stdx {
inline namespace v1 {
template <typename Key, typename Value, std::size_t N> class cx_map {
  public:
    using key_type = Key;
    using mapped_type = Value;
    struct value_type {
        key_type key{};
        mapped_type value{};
    };
    using size_type = std::size_t;
    using reference = value_type &;
    using const_reference = value_type const &;
    using iterator = value_type *;
    using const_iterator = value_type const *;

  private:
    std::array<value_type, N> storage{};
    std::size_t current_size{};

  public:
    constexpr cx_map() = default;
    template <typename... Vs, std::enable_if_t<((sizeof...(Vs) <= N) and ... and
                                                stdx::same_as<value_type, Vs>),
                                               int> = 0>
    constexpr explicit cx_map(Vs const &...vs)
        : storage{vs...}, current_size{sizeof...(Vs)} {}

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

    [[nodiscard]] constexpr auto size() const -> std::size_t {
        return current_size;
    }
    constexpr static std::integral_constant<size_type, N> capacity{};

    [[nodiscard]] constexpr auto full() const -> bool {
        return current_size == N;
    }
    [[nodiscard]] constexpr auto empty() const -> bool {
        return current_size == 0;
    }

    constexpr auto clear() -> void { current_size = 0; }

    [[nodiscard]] constexpr auto pop_back() -> value_type {
        return storage[--current_size];
    }

    [[nodiscard]] constexpr auto get(key_type const &key) -> mapped_type & {
        for (auto &[k, v] : *this) {
            if (k == key) {
                return v;
            }
        }
        unreachable();
    }
    [[nodiscard]] constexpr auto get(key_type const &key) const
        -> mapped_type const & {
        for (auto const &[k, v] : *this) {
            if (k == key) {
                return v;
            }
        }
        unreachable();
    }

    [[nodiscard]] constexpr auto contains(key_type const &key) const -> bool {
        for (auto const &[k, v] : *this) {
            if (k == key) {
                return true;
            }
        }
        return false;
    }

    constexpr auto insert_or_assign(key_type const &key,
                                    mapped_type const &value) -> bool {
        for (auto &[k, v] : *this) {
            if (k == key) {
                v = value;
                return false;
            }
        }
        storage[current_size++] = {key, value};
        return true;
    }
    constexpr auto put(key_type const &key, mapped_type const &value) -> bool {
        return insert_or_assign(key, value);
    }

    constexpr auto erase(key_type const &key) -> size_type {
        for (auto &v : *this) {
            if (v.key == key) {
                v = storage[--current_size];
                return 1u;
            }
        }
        return 0u;
    }
};
} // namespace v1
} // namespace stdx
