/// \file
#pragma once

#include <memory>
#include <type_traits>

#include <utils/uncopyable.hpp>
#include <bsod/bsod.h>

template <typename Base = void>
struct InplaceAnyRTTI {
    void (*destructor)(void *);
    Base *(*base_cast)(void *);
};

template <typename T, typename Base = void>
static consteval const InplaceAnyRTTI<Base> *inplace_any_rtti() {
    static constexpr InplaceAnyRTTI<Base> result {
        .destructor = [](void *data) { std::destroy_at<T>(reinterpret_cast<T *>(data)); },
        .base_cast = [](void *data) { return static_cast<Base *>(reinterpret_cast<T *>(data)); },
    };
    return &result;
}

// Sanity check that we're really creating unique pointer for each type
static_assert(inplace_any_rtti<uint32_t>() != inplace_any_rtti<uint8_t>());

/// Alternative to std::any that never dynamically allocates
/// Cannot be moved though
/// @tparam Base_ Optional common base class; enables get_base_if() for type-erased access
template <size_t storage_size, typename Alignment_ = void *, typename Base_ = void>
class InplaceAny : public Uncopyable {

public:
    using Alignment = Alignment_;
    using Base = Base_;

public:
    constexpr InplaceAny() = default;

    ~InplaceAny() {
        reset();
    }

public:
    /// Constructs the provided type inside the storage. Destroys what was previously there.
    template <typename T, typename... Args>
    constexpr T &emplace(Args &&...args) {
        static_assert(sizeof(T) <= storage_size, "T does not fit into InplaceAny storage");
        static_assert(std::alignment_of_v<T> <= std::alignment_of_v<Alignment>, "T requires higher alignment");

        reset();
        type_ = inplace_any_rtti<T, Base>();
        return *std::construct_at<T, Args...>(reinterpret_cast<T *>(data_.data()), std::forward<Args>(args)...);
    }

    /// Destroys whatever is in the storage
    constexpr void reset() {
        if (type_) {
            type_->destructor(data_.data());
            type_ = nullptr;
        }
    }

public:
    /// \returns reference to the value of type T. Assumes the Any holds the correct type.
    template <typename T>
    constexpr inline T &get() {
        debug_assert(holds_alternative<T>());
        return *reinterpret_cast<T *>(data_.data());
    }

    /// \returns reference to the value of type T. Assumes the Any holds the correct type.
    template <typename T>
    constexpr inline const T &get() const {
        debug_assert(holds_alternative<T>());
        return *reinterpret_cast<const T *>(data_.data());
    }

    /// \returns pointer to the value of type T, if the InplaceAny is of the provided type, otherwise nullptr
    template <typename T>
    constexpr inline T *get_if() {
        return holds_alternative<T>() ? &get<T>() : nullptr;
    }

    /// \returns pointer to the value of type T, if the InplaceAny is of the provided type, otherwise nullptr
    template <typename T>
    constexpr inline const T *get_if() const {
        return holds_alternative<T>() ? &get<T>() : nullptr;
    }

    /// @returns pointer to common Base class if has_value(), otherwise nullptr
    constexpr Base *get_base_if() {
        return has_value() ? type_->base_cast(reinterpret_cast<void *>(data_.data())) : nullptr;
    }

    /// \returns if the variant holds alternative of the given type
    template <typename T>
    constexpr inline bool holds_alternative() const {
        return type_ == inplace_any_rtti<T, Base>();
    }

    constexpr inline bool has_value() const {
        return type_ != nullptr;
    }

private:
    /// Contents of the variant
    alignas(Alignment) std::array<uint8_t, storage_size> data_ = { 0 };

    /// Pointer representing the type
    const InplaceAnyRTTI<Base> *type_ = nullptr;
};
