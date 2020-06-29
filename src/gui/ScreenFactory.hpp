#pragma once

#include "screen_home.hpp"
#include "static_alocation_ptr.hpp"
#include <array>

class ScreenFactory {
    ScreenFactory() = delete;
    ScreenFactory(const ScreenFactory &) = delete;
    using mem_space = std::aligned_union<0, screen_home_data_t>::type;
    static mem_space all_screens;

public:
    using unique_ptr = static_unique_ptr<window_frame_t>;
    using fnc = static_unique_ptr<window_frame_t> (*)(); //function pointer definition

    unique_ptr ScreenHome() {
        return make_static_unique_ptr<screen_home_data_t>(&all_screens);
    }
};
