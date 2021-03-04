#pragma once

#include "screen_home.hpp"
#include "screen_splash.hpp"
#include "static_alocation_ptr.hpp"
#include <array>

class ScreenFactory {
    ScreenFactory() = delete;
    ScreenFactory(const ScreenFactory &) = delete;
    using mem_space = std::aligned_union<4096, screen_home_data_t, screen_splash_data_t>::type;
    static mem_space all_screens;

public:
    using UniquePtr = static_unique_ptr<screen_t>;
    using Creator = static_unique_ptr<screen_t> (*)(); //function pointer definition

    template <class T>
    static UniquePtr Screen() {
        static_assert(sizeof(T) <= sizeof(mem_space), "Screen memory space is too small");
        return make_static_unique_ptr<T>(&all_screens);
    }
};
