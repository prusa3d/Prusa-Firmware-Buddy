#pragma once

/// Tri-state "bool" - off, on and third "other" state (undefined/middle/...)
struct Tristate {

public:
    enum Value {
        no = 0,
        yes = 1,
        other = 2
    };

public:
    constexpr inline Tristate() = default;
    constexpr inline Tristate(const Tristate &) = default;

    constexpr inline Tristate(Value val)
        : value(val) {}

    /// Implicit constructor from bool
    constexpr inline Tristate(bool val)
        : value(static_cast<Value>(val)) {}

    constexpr static inline Tristate from_bool(bool value) {
        return Tristate(value);
    }

    /// Implicit cast to bool - \p other state translates as false
    constexpr inline operator bool() const {
        return value == Value::yes;
    }

    constexpr inline Tristate &operator=(const Tristate &) = default;

    constexpr inline bool operator==(const Tristate &) const = default;
    constexpr inline bool operator!=(const Tristate &) const = default;

    constexpr inline bool operator==(Value o) const {
        return value == o;
    }
    constexpr inline bool operator!=(Value o) const {
        return value != o;
    }

public:
    Value value = Value::other;
};
