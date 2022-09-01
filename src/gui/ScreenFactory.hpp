#pragma once

#include "screen_home.hpp"
#include "screen_splash.hpp"
#include "static_alocation_ptr.hpp"
#include <array>

class ScreenFactory {
    // menu settings is the biggest, but I cannot access it from here (to use it as argument of aligned_union)
    // so i have to define minimal size like this
    //to calculate the size use this char (*size_msg_as_error)[sizeof( ScreenMenuSettings )] = 1;
#if _DEBUG
    static constexpr size_t min_union_size = 3620;
#else
    static constexpr size_t min_union_size = 2840;
#endif

    ScreenFactory() = delete;
    ScreenFactory(const ScreenFactory &) = delete;
    using mem_space = std::aligned_union<min_union_size, screen_home_data_t, screen_splash_data_t>::type;
    static mem_space all_screens;

public:
    using UniquePtr = static_unique_ptr<screen_t>;
    using Creator = static_unique_ptr<screen_t> (*)(); //function pointer definition

    template <class T>
    static UniquePtr Screen() {
        static_assert(sizeof(T) <= sizeof(mem_space), "Screen memory space is too small");
        return make_static_unique_ptr<T>(&all_screens);
    }

    template <class T>
    static bool DoesCreatorHoldType(Creator cr) {
        return Screen<T> == cr;
    }
};
