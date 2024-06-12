#pragma once

#include <type_traits>
#include <array>

struct PrimitiveAnyRTTI {
    const void *ptr = nullptr;

    constexpr bool operator==(const PrimitiveAnyRTTI &) const = default;
    constexpr bool operator!=(const PrimitiveAnyRTTI &) const = default;
};

template <typename T>
consteval PrimitiveAnyRTTI primitive_any_rtti() {
    // Unique pointer for each type
    static constexpr bool ptr = false;
    return { &ptr };
}

// Some basic sanity checks
static_assert(primitive_any_rtti<void>() != primitive_any_rtti<uint32_t>());
static_assert(primitive_any_rtti<void>() == primitive_any_rtti<void>());

/// Alternative to std::any that:
/// - never dynamically allocates
/// - has a primitive destructor
template <size_t max_size>
class PrimitiveAny {

public:
    constexpr PrimitiveAny() = default;

    template <size_t other_size>
    constexpr inline PrimitiveAny(const PrimitiveAny<other_size> &other) {
        operator=(other);
    }

    template <typename T>
    constexpr inline PrimitiveAny(const T &value) {
        operator=(value);
    }

    constexpr ~PrimitiveAny() = default;

public:
    /// \returns pointer to the value of type T, if the Variant is of this type (or nullptr)
    template <typename T>
    constexpr T *get_maybe() {
        return (type == primitive_any_rtti<T>()) ? reinterpret_cast<T *>(data.data()) : nullptr;
    }

    /// \returns pointer to the value of type T, if the Variant is of this type (or nullptr)
    template <typename T>
    constexpr const T *get_maybe() const {
        return (type == primitive_any_rtti<T>()) ? reinterpret_cast<const T *>(data.data()) : nullptr;
    }

    /// \returns value of T, if the variant holds this alternative, or \p fallback
    template <typename T>
    constexpr const T get_or(const T &fallback) const {
        if (auto r = get_maybe<T>()) {
            return *r;
        } else {
            return fallback;
        }
    }

    /// Sets the variant to a given value
    template <typename T>
    constexpr void set(const T &value) {
        // If T is not trivially destructible, we need to add support for it in this class destructor
        static_assert(std::is_trivially_destructible_v<T>);

        // Ditto for copying
        static_assert(std::is_trivially_copyable_v<T>);

        type = primitive_any_rtti<T>();
        new (data.data()) T(value);
    }

    /// \returns if the variant holds alternative of the given type
    template <typename T>
    constexpr bool holds_alternative() const {
        return type == primitive_any_rtti<T>();
    }

    template <size_t other_size>
    constexpr inline PrimitiveAny &operator=(const PrimitiveAny<other_size> &other) {
        static_assert(max_size >= other_size);
        type = other.type;
        memcpy(data.data(), other.data.data(), max_size);
        return *this;
    }

    template <typename T>
    constexpr inline PrimitiveAny &operator=(const T &value) {
        set(value);
        return *this;
    }

private:
    /// Contents of the variant
    std::array<uint8_t, max_size> data = { 0 };

    /// Pointer representing the type
    PrimitiveAnyRTTI type;
};
