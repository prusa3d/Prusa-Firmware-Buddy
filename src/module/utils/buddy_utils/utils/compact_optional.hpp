/// @file
#pragma once

#include <optional>
#include <cmath>
#include <cstring>

#include <bsod/bsod.h>

namespace compact_optional {
/// Default has_value implementation that uses the comparison operator
template <typename T, T nullopt_value>
constexpr bool has_value_impl(const T &val) {
    return val != nullopt_value;
}

/// For floats, NAN != NAN, so we have to use memcmp
template <typename T, T nullopt_value>
constexpr bool has_value_impl(const T &val)
    requires(std::is_floating_point_v<T> && std::isnan(nullopt_value))
{
    static_assert(sizeof(T) == sizeof(nullopt_value));
    static constexpr auto nullopt_val_var = nullopt_value;
    return memcmp(&val, &nullopt_val_var, sizeof(T)) != 0;
}

/// Necessary wrapper so that compact_optional::has_value<T, nullopt_value_> wouldn't be ambiguous
template <typename T, T nullopt_value>
constexpr bool has_value(const T &val) {
    return has_value_impl<T, nullopt_value>(val);
}

}; // namespace compact_optional

/// std::optional alternative that doesn't take an extra byte
/// but instead encodes the null as @p null_value
/// @p null_value is then not permitted
template <typename T, T nullopt_value_, auto has_value_f = compact_optional::has_value<T, nullopt_value_>>
struct CompactOptional {

public:
    static constexpr T nullopt_value = nullopt_value_;

    constexpr CompactOptional()
        : value_(nullopt_value) {}

    constexpr CompactOptional(std::nullopt_t)
        : value_(nullopt_value) {}

    constexpr CompactOptional(const CompactOptional &) = default;
    constexpr CompactOptional(CompactOptional &&) = default;

    constexpr CompactOptional(const T &value)
        : value_(value) {
        enforce_has_value();
    }

    explicit constexpr CompactOptional(T &&value)
        : value_(std::move(value)) {
        enforce_has_value();
    }

    constexpr CompactOptional(const std::optional<T> &o)
        : value_(o.value_or(nullopt_value)) {
    }

    constexpr bool has_value() const {
        return has_value_f(value_);
    }

    constexpr const T &value() const {
        enforce_has_value();
        return value_;
    }

    constexpr T value_or(const T &fallback) const {
        return has_value() ? value_ : fallback;
    }

    constexpr const T &operator*() const {
        enforce_has_value();
        return value_;
    }

    constexpr const T *operator->() const {
        enforce_has_value();
        return &value_;
    }

    constexpr CompactOptional &operator=(const CompactOptional &) = default;
    constexpr CompactOptional &operator=(CompactOptional &&) = default;

    constexpr CompactOptional &operator=(const T &o) {
        value_ = o;
        enforce_has_value();
        return *this;
    }
    constexpr CompactOptional &operator=(T &&o) {
        value_ = std::move(o);
        enforce_has_value();
        return *this;
    }

    constexpr bool operator==(const CompactOptional &o) const {
        const bool has_value = this->has_value();

        // Note: cannot use default operator== because has_value_f might not be a simple comparison
        if (has_value != o.has_value()) {
            return false;

        } else if (!has_value) {
            return true;

        } else {
            return value_ == o.value_;
        }
    }

    constexpr bool operator!=(const CompactOptional &o) const {
        return !operator==(o);
    }

    constexpr bool operator==(const T &o) const {
        if (!has_value()) {
            return false;
        }
        return value() == o;
    }
    constexpr bool operator!=(const T &o) const {
        return !operator==(o);
    }

    constexpr explicit operator bool() const
        requires(!std::is_same_v<T, bool>)
    {
        return has_value();
    }

    constexpr operator std::optional<T>() const {
        return has_value() ? std::make_optional(value_) : std::nullopt;
    }

private:
    constexpr void enforce_has_value() const {
        if (!has_value()) {
            bsod_unreachable();
        }
    }

private:
    T value_ = nullopt_value;
};
