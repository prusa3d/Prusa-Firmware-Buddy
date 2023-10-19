#pragma once

#include <stdx/concepts.hpp>
#include <stdx/cx_map.hpp>

#include <cstddef>

namespace stdx {
inline namespace v1 {
template <typename Key, std::size_t N> class cx_set {
    std::array<Key, N> storage{};
    std::size_t current_size{};

  public:
    using key_type = Key;
    using value_type = Key;
    using size_type = std::size_t;
    using reference = value_type &;
    using const_reference = value_type const &;
    using iterator = value_type *;
    using const_iterator = value_type const *;

    constexpr cx_set() = default;
    template <typename... Ts,
              std::enable_if_t<((sizeof...(Ts) <= N) and ... and
                                stdx::convertible_to<key_type, Ts>),
                               int> = 0>
    constexpr explicit cx_set(Ts const &...ts)
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

    [[nodiscard]] constexpr auto size() const -> size_type {
        return current_size;
    }
    constexpr static std::integral_constant<size_type, N> capacity{};

    [[nodiscard]] constexpr auto contains(key_type const &key) const -> bool {
        for (auto const &k : *this) {
            if (k == key) {
                return true;
            }
        }
        return false;
    }

    constexpr auto insert(key_type const &key) -> bool {
        if (contains(key)) {
            return false;
        }
        storage[current_size++] = key;
        return true;
    }
    constexpr auto insert(key_type &&key) -> bool {
        if (contains(key)) {
            return false;
        }
        storage[current_size++] = std::move(key);
        return true;
    }

    constexpr auto erase(key_type const &key) -> size_type {
        for (auto &k : *this) {
            if (k == key) {
                k = storage[--current_size];
                return 1u;
            }
        }
        return 0u;
    }

    template <typename Set> constexpr auto merge(Set s) -> void {
        for (auto const &entry : s) {
            insert(entry);
        }
    }

    [[nodiscard]] constexpr auto empty() const -> bool {
        return current_size == 0u;
    }

    constexpr auto clear() -> void { current_size = 0; }

    [[nodiscard]] constexpr auto pop_back() -> key_type {
        return storage[--current_size];
    }
};

template <typename T, typename... Ts>
cx_set(T, Ts...) -> cx_set<T, 1 + sizeof...(Ts)>;
} // namespace v1
} // namespace stdx
