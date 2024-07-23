#pragma once

#include <static_alocation_ptr.hpp>
#include <screen.hpp>
#include <array>
#include <config.h>

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
    using Creator = static_unique_ptr<screen_t> (*)(); // function pointer definition

    template <class T, auto... args>
    static UniquePtr Screen() {
        static_assert(sizeof(T) <= storage.size(), "Screen memory space is too small");
        return make_static_unique_ptr<T>(storage.data(), args...);
    }

    template <class T>
    static bool DoesCreatorHoldType(Creator cr) {
        return Screen<T> == cr;
    }
};
