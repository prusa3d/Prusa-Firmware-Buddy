/**
 * @file screen_saver.hpp
 * @author Radek Vana
 * @brief screen saver - to be used while printing
 * @date 2021-03-04
 */
#pragma once

#include "DialogTimed.hpp"
#include "window_text.hpp"
class ScreenSaver : public AddSuperWindow<TimedDialog> {
    static constexpr uint32_t open_period = 30000; //ms
    window_text_t dummy_txt;

public:
    ScreenSaver(); // no parent pointer in ctor, registered manually
};
