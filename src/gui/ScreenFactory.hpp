#pragma once

#include <static_alocation_ptr.hpp>
#include <screen.hpp>
#include <array>
#include <config.h>
#include <common/primitive_any.hpp>

class ScreenFactory {
    ScreenFactory() = delete;
    ScreenFactory(const ScreenFactory &) = delete;

#if PRINTER_IS_PRUSA_XL()
    using Storage = std::array<uint8_t, 4096>;
#elif PRINTER_IS_PRUSA_MINI()
    using Storage = std::array<uint8_t, 3072>;
#else
    using Storage = std::array<uint8_t, 4096>;
#endif
    alignas(std::max_align_t) static Storage storage;

public:
    using UniquePtr = static_unique_ptr<screen_t>;

    struct Creator {
        using Arg = PrimitiveAny<4>;
        using Func = UniquePtr (*)(const Arg &arg);

        constexpr Creator() = default;

        constexpr Creator(Func func, const Arg &arg = {})
            : func(func)
            , arg(arg) {}

        constexpr inline UniquePtr operator()() const {
            return func(arg);
        }

        constexpr inline bool operator==(const Creator &) const = default;
        constexpr inline bool operator!=(const Creator &) const = default;

        Func func = nullptr;
        Arg arg;
    };

    template <class T, auto... args>
    static UniquePtr Screen(const Creator::Arg & = {}) {
        // Note: the Arg must be in the function so that the function prototype matches Creator::Func
        static_assert(sizeof(T) <= storage.size(), "Screen memory space is too small");
        return make_static_unique_ptr<T>(storage.data(), args...);
    }

    template <class T, typename Arg>
    static inline Creator ScreenWithArg(Arg arg) {
        static_assert(sizeof(T) <= storage.size(), "Screen memory space is too small");
        static constexpr auto ctor = +[](const Creator::Arg &arg_variant) -> UniquePtr {
            return make_static_unique_ptr<T>(storage.data(), arg_variant.value<Arg>());
        };
        return Creator(ctor, Creator::Arg::make(arg));
    }

    template <class T>
    static bool DoesCreatorHoldType(Creator cr) {
        return Creator(Screen<T>) == cr;
    }
};
