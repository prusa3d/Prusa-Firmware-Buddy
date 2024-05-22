#pragma once

#include <stdx/cx_map.hpp>
#include <stdx/cx_set.hpp>

#include <cstddef>
#include <iterator>
#include <type_traits>

namespace stdx {
inline namespace v1 {
template <typename Key, typename Value, std::size_t KeyN,
          std::size_t ValueN = KeyN>
class cx_multimap {
    using set_t = cx_set<Value, ValueN>;
    using storage_t = cx_map<Key, set_t, KeyN>;
    storage_t storage{};

  public:
    using key_type = Key;
    using mapped_type = Value;
    using size_type = std::size_t;
    using iterator = typename storage_t::iterator;
    using const_iterator = typename storage_t::const_iterator;

    [[nodiscard]] constexpr auto begin() -> iterator {
        return std::begin(storage);
    }
    [[nodiscard]] constexpr auto begin() const -> const_iterator {
        return std::begin(storage);
    }
    [[nodiscard]] constexpr auto cbegin() const -> const_iterator {
        return std::cbegin(storage);
    }

    [[nodiscard]] constexpr auto end() -> iterator { return std::end(storage); }
    [[nodiscard]] constexpr auto end() const -> const_iterator {
        return std::end(storage);
    }
    [[nodiscard]] constexpr auto cend() const -> const_iterator {
        return std::cend(storage);
    }

    [[nodiscard]] constexpr auto size() const -> std::size_t {
        return std::size(storage);
    }
    constexpr static std::integral_constant<size_type, KeyN> capacity{};

    constexpr auto insert(key_type const &k, mapped_type const &v) -> void {
        if (storage.contains(k)) {
            storage.get(k).insert(v);
        } else {
            storage.insert_or_assign(k, set_t{v});
        }
    }
    constexpr auto insert(key_type const &k) -> void {
        if (not storage.contains(k)) {
            storage.insert_or_assign(k, set_t{});
        }
    }
    template <typename... Args> constexpr auto put(Args &&...args) -> void {
        return insert(std::forward<Args>(args)...);
    }

    constexpr auto erase(key_type const &key) -> size_type {
        return storage.erase(key);
    }
    constexpr auto erase(key_type const &k, mapped_type const &v) -> size_type {
        if (storage.contains(k)) {
            auto &s = storage.get(k);
            auto const r = s.erase(v);
            if (s.empty()) {
                storage.erase(k);
            }
            return r;
        }
        return 0;
    }

    [[nodiscard]] constexpr auto get(key_type const &key) -> set_t & {
        return storage.get(key);
    }
    [[nodiscard]] constexpr auto get(key_type const &key) const
        -> set_t const & {
        return storage.get(key);
    }

    [[nodiscard]] constexpr auto empty() const -> bool {
        return storage.empty();
    }

    [[nodiscard]] constexpr auto contains(key_type const &key) const -> bool {
        return storage.contains(key);
    }
    [[nodiscard]] constexpr auto contains(key_type const &k,
                                          mapped_type const &v) const -> bool {
        return contains(k) and get(k).contains(v);
    }

    constexpr auto clear() -> void { storage.clear(); }
};
} // namespace v1
} // namespace stdx
