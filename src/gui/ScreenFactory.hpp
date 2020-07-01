#pragma once

#include "screen_home.hpp"
#include "screen_splash.hpp"
#include "static_alocation_ptr.hpp"
#include <array>

class ScreenFactory {
    ScreenFactory() = delete;
    ScreenFactory(const ScreenFactory &) = delete;
    using mem_space = std::aligned_union<0, screen_home_data_t, screen_splash_data_t>::type;
    static mem_space all_screens;

public:
    using UniquePtr = static_unique_ptr<window_frame_t>;
    using Creator = static_unique_ptr<window_frame_t> (*)(); //function pointer definition

    static UniquePtr ScreenHome();
    static UniquePtr ScreenSplash();
};
