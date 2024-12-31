#pragma once

#include <type_traits>
#include <array>
#include <assert.h>

using PrimitiveAnyAlignment = void *;

struct PrimitiveAnyRTTI {

public:
    template <typename T>
    static consteval const PrimitiveAnyRTTI *get_for_type() {
        static_assert(std::alignment_of_v<T> <= std::alignment_of_v<PrimitiveAnyAlignment>);

        // PrimitiveAny would need a more complex implementation if we were to support non-trivial copies and destructors
        static_assert(std::is_trivially_destructible_v<T>);
        static_assert(std::is_trivially_copyable_v<T>);

        // Unique pointer for each instance
        static constexpr PrimitiveAnyRTTI data {};
        return &data;
    }

private:
    // Make sure PrimitiveAnyRTTI has sizeof() > 0
    [[maybe_unused]] uint8_t data;
};

// Sanity check that we're really creating unique pointer for each type
static_assert(PrimitiveAnyRTTI::get_for_type<uint32_t>() != PrimitiveAnyRTTI::get_for_type<uint8_t>());

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
    static constexpr inline PrimitiveAny make(const T &value) {
        PrimitiveAny r;
        r.set(value);
        return r;
    }

    constexpr ~PrimitiveAny() = default;

public:
    /// \returns pointer to the value of type T, if the Variant is of this type (or nullptr)
    template <typename T>
    constexpr T *value_maybe() {
        return (type == PrimitiveAnyRTTI::get_for_type<T>()) ? reinterpret_cast<T *>(data.data()) : nullptr;
    }

    /// \returns pointer to the value of type T, if the Variant is of this type (or nullptr)
    template <typename T>
    constexpr const T *value_maybe() const {
        return (type == PrimitiveAnyRTTI::get_for_type<T>()) ? reinterpret_cast<const T *>(data.data()) : nullptr;
    }

    /// \returns value of T, if the variant holds this alternative, or \p fallback
    template <typename T, typename Fallback>
    constexpr const T value_or(const Fallback &fallback) const {
        // !!! It is important to have T and Fallback separate to prevent automatic T inferation
        if (auto r = value_maybe<T>()) {
            return *r;
        } else {
            return fallback;
        }
    }

    /// \returns reference to the value of type T. Undefined behavior if the PrimitveAny does not hold the value.
    template <typename T>
    constexpr T &value() {
        assert(holds_alternative<T>());
        return *value_maybe<T>();
    }

    /// \returns reference to the value of type T. Undefined behavior if the PrimitveAny does not hold the value.
    template <typename T>
    constexpr const T &value() const {
        assert(holds_alternative<T>());
        return *value_maybe<T>();
    }

    /// Sets the variant to a given value
    template <typename T>
    constexpr void set(const T &value) {
        static_assert(sizeof(T) <= max_size);
        type = PrimitiveAnyRTTI::get_for_type<T>();
        new (data.data()) T(value);
    }

    /// \returns if the variant holds alternative of the given type
    template <typename T>
    constexpr bool holds_alternative() const {
        return type == PrimitiveAnyRTTI::get_for_type<T>();
    }

    constexpr inline bool has_value() const {
        return type != nullptr;
    }

    constexpr inline operator bool() const {
        return has_value();
    }
    constexpr inline bool operator!() const {
        return !has_value();
    }

    bool operator==(const PrimitiveAny &o) const {
        return (type == o.type) && (!type || data == o.data);
    }
    bool operator!=(const PrimitiveAny &o) const {
        return !(*this == o);
    }

    template <size_t other_size>
    constexpr inline PrimitiveAny &operator=(const PrimitiveAny<other_size> &other) {
        static_assert(max_size >= other_size);
        type = other.type;
        memcpy(data.data(), other.data.data(), std::min(max_size, other_size));
        return *this;
    }

private:
    /// Contents of the variant
    alignas(PrimitiveAnyAlignment) std::array<uint8_t, max_size> data = { 0 };

    /// Pointer representing the type
    const PrimitiveAnyRTTI *type = nullptr;
};
