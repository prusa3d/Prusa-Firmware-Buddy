/// @file
#pragma once

#include <cmath>
#include <cstddef>
#include <functional>
#include <concepts>
#include <iterator>

#include <utils/meta/meta_utils.hpp>

template <typename T, typename Tag, size_t size_>
struct Vector {

public:
    // ===========================
    // Basic scaffolding
    // ===========================

    static constexpr size_t size() {
        return size_;
    };

    // Note: Cannot use std::monostate, each component must be a different type
    // [[no_unique_address]] cannot consolidate same types
    template <size_t i>
    struct UnusedComponent {
        friend struct Vector;

    public:
        constexpr UnusedComponent() = default;

    private:
        // Try to prevent accidentally using UnusedComponent, for example vector_xy.z = vector2_xy.z;
        constexpr UnusedComponent(const UnusedComponent &) = default;
        constexpr UnusedComponent &operator=(const UnusedComponent &) = default;
    };

    template <size_t i>
    using ComponentType = std::conditional_t<(size() > i), T, UnusedComponent<i>>;

    template <size_t i>
    static consteval auto component_init_val() {
        if constexpr (size() > i) {
            return T { 0 };
        } else {
            return UnusedComponent<i> {};
        }
    }

    union {
        struct {
            [[no_unique_address]] ComponentType<0> x = component_init_val<0>();
            [[no_unique_address]] ComponentType<1> y = component_init_val<1>();
            [[no_unique_address]] ComponentType<2> z = component_init_val<2>();
            [[no_unique_address]] ComponentType<3> e = component_init_val<3>();
            static_assert(size() < 5); // All components must be default-initialized
        };

        // Alternative component names
        struct {
            [[no_unique_address]] ComponentType<0> a;
            [[no_unique_address]] ComponentType<1> b;
            [[no_unique_address]] ComponentType<2> c;
            [[no_unique_address]] ComponentType<3> _e;
        };

        T pos[size()];
    };

    [[nodiscard]] [[gnu::always_inline]] constexpr T &operator[](size_t i) {
        return pos[i];
    }
    [[nodiscard]] [[gnu::always_inline]] constexpr const T &operator[](size_t i) const {
        // Check that the Vector has the size we expect
        // We cannot put this into the class scope as sizeof() is not known at that point
        static_assert(sizeof(Vector<T, Tag, size_>) == sizeof(T) * size_);

        return pos[i];
    }

    /// @returns nth component of the vector
    template <size_t i>
    [[nodiscard]] [[gnu::always_inline]] constexpr const T &at() const {
        static_assert(i < size());

        // Note: You might be wondering why we just don't use pos[i]
        // Turns out this takes less flash
        if constexpr (i == 0) {
            return x;
        } else if constexpr (i == 1) {
            return y;
        } else if constexpr (i == 2) {
            return z;
        } else if constexpr (i == 3) {
            return e;
        } else {
            static_assert(false);
        }
    }

    /// @returns nth component of the vector
    template <size_t i>
    [[nodiscard]] [[gnu::always_inline]] constexpr T &at() {
        static_assert(i < size());

        // Note: You might be wondering why we just don't use pos[i]
        // Turns out this takes less flash
        if constexpr (i == 0) {
            return x;
        } else if constexpr (i == 1) {
            return y;
        } else if constexpr (i == 2) {
            return z;
        } else if constexpr (i == 3) {
            return e;
        } else {
            static_assert(false);
        }
    }

    [[nodiscard]] constexpr auto begin() {
        return std::begin(pos);
    }
    [[nodiscard]] constexpr auto begin() const {
        return std::begin(pos);
    }

    [[nodiscard]] constexpr auto end() {
        return std::end(pos);
    }
    [[nodiscard]] constexpr auto end() const {
        return std::end(pos);
    }

public:
    // ===========================
    // Setters
    // ===========================

    // Assignment operator overrides do the expected thing
    constexpr Vector &operator=(const Vector &) = default;

    // Don't use these, they are dangerous. Always retype explicitly.
    // Note: the concept + auto is required for the operator=(Vector) to take precedence
    constexpr Vector &operator=(std::convertible_to<T> auto &v) = delete;

    /// Was present in previous implementation. Explicitly deleting it to make sure it would not get resolved.
    template <size_t size2>
    void set(const T (&)[size2]) = delete;

    /// Sets first N components to the provided values
    template <std::convertible_to<T>... Args>
    constexpr void set(Args... args) {
        static_assert(sizeof...(args) <= size(), "Provided more components than vector size");
        static_assert(sizeof...(args) > 1, "set(x) would be ambiguous - would we be setting a first component, or all components to the value?");

        return [&]<size_t... i>(std::index_sequence<i...>) {
            ((at<i>() = nth_argument<i>(args...)), ...);
        }(std::make_index_sequence<sizeof...(args)> {});
    }

    /// Sets first N components to the provided vector values,
    /// can be followed by scalar values
    template <size_t size2, std::convertible_to<T>... Args>
    constexpr void set(const Vector<T, Tag, size2> &o, Args... args) {
        static_assert(size2 + sizeof...(args) <= size(), "Provided more components than vector size");

        [&]<size_t... i>(std::index_sequence<i...>) {
            ((at<i>() = o.template at<i>()), ...);
        }(std::make_index_sequence<size2> {});

        [&]<size_t... i>(std::index_sequence<i...>) {
            ((at<size2 + i>() = nth_argument<i>(args...)), ...);
        }(std::make_index_sequence<sizeof...(args)> {});
    }

public:
    // ===========================
    // Upcasts/downcasts/casts
    // ===========================

    /// Explicit upcast operator
    template <size_t size2>
    explicit operator Vector<T, Tag, size2>() const
        requires(size2 > size())
    {
        return [&]<size_t... i>(std::index_sequence<i...>) {
            return Vector<T, Tag, size2> { at<i>()... };
        }(std::make_index_sequence<size()> {});
    }

    /// Basically a reinterpret_cast for the vectors. Only use when you know what you're doing
    template <typename NewTag>
    [[deprecated("UNSAFE. Only use when you know what you're doing")]] [[nodiscard]] constexpr auto to_tag() const {
        return [&]<size_t... i>(std::index_sequence<i...>) {
            return Vector<T, NewTag, size_> { at<i>()... };
        }(std::make_index_sequence<size()> {});
    }

    // Cast to types with fewer components
    [[nodiscard]] constexpr Vector<T, Tag, 2> xy() const
        requires(size() > 2)
    {
        return { .x = x, .y = y };
    }

    [[nodiscard]] constexpr Vector<T, Tag, 3> xyz() const
        requires(size() > 3)
    {
        return { .x = x, .y = y, .z = z };
    }

    template <typename = void>
    [[nodiscard]] constexpr auto asLogical() const {
        return toLogical(*this);
    }

    template <typename = void>
    [[nodiscard]] constexpr auto asNative() const {
        return toNative(*this);
    }

public:
    // ===========================
    // Mathematical operations
    // ===========================

    /// @returns a vector where each component r[i] = op(a[i], b[i])
    /// The other vector can be smaller, in which case the remaining components are left untouched
    template <size_t size2>
    [[nodiscard]] [[gnu::always_inline]] constexpr Vector component_wise_binary_op(const Vector<T, Tag, size2> &other, auto &&op) const {
        static_assert(size() >= size2, "Right operand vector is bigger than left operand");

        return [&]<size_t... i, size_t... i2>(std::index_sequence<i...>, std::index_sequence<i2...>) {
            return Vector { T(op(at<i>(), other.template at<i>()))..., at<size2 + i2>()... };
        }(std::make_index_sequence<size2> {}, std::make_index_sequence<size() - size2> {});
    }

    /// @returns a vector where each component r[i] = op(a[i], val)
    /// !!! That std::convertible_to<T> auto val is necessary so that if we do (int * float) for example, the computation happens as floats and THEN is casted
    [[nodiscard]] [[gnu::always_inline]] constexpr Vector component_wise_binary_op(std::convertible_to<T> auto val, auto &&op) const {
        return [&]<size_t... i>(std::index_sequence<i...>) {
            return Vector { T(op(at<i>(), val))... };
        }(std::make_index_sequence<size()> {});
    }

    template <typename = void>
    [[nodiscard]] constexpr T magnitude() const {
        return [&]<size_t... i>(std::index_sequence<i...>) {
            return sqrtf(((at<i>() * at<i>()) + ...));
        }(std::make_index_sequence<size()> {});
    }

    // Override other operators to get intuitive behaviors
    [[nodiscard]] constexpr Vector operator+(const auto &rs) const {
        return component_wise_binary_op(rs, std::plus {});
    }
    constexpr Vector &operator+=(const auto &rs) {
        *this = this->operator+(rs);
        return *this;
    }

    [[nodiscard]] constexpr Vector operator-(const auto &rs) const {
        return component_wise_binary_op(rs, std::minus {});
    }
    constexpr Vector &operator-=(const auto &rs) {
        *this = this->operator-(rs);
        return *this;
    }

    [[nodiscard]] constexpr const Vector operator-() const {
        return [&]<size_t... i>(std::index_sequence<i...>) {
            return Vector { -at<i>()... };
        }(std::make_index_sequence<size()> {});
    }

    [[nodiscard]] constexpr Vector operator*(const auto &rs) const {
        return component_wise_binary_op(rs, std::multiplies {});
    }
    constexpr Vector &operator*=(const auto &rs) {
        *this = this->operator*(rs);
        return *this;
    }

    [[nodiscard]] constexpr Vector operator/(const auto &rs) const {
        return component_wise_binary_op(rs, std::divides {});
    }
    constexpr Vector &operator/=(const auto &rs) {
        *this = this->operator/(rs);
        return *this;
    }

    [[nodiscard]] constexpr bool operator==(const Vector &o) const {
        return [&]<size_t... i>(std::index_sequence<i...>) {
            return ((at<i>() == o.template at<i>()) && ...);
        }(std::make_index_sequence<size()> {});
    }
};
