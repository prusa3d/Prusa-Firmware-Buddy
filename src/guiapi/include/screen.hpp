/**
 * @file screen.hpp
 * @brief abstract screen to inherit from
 * one screen is opened at the time
 * open method must pe overloaded
 */

#pragma once

#include "i_screen.hpp"
#include "static_alocation_ptr.hpp"

class screen_t : public IScreen {

public:
    using UniquePtr = static_unique_ptr<screen_t>;
    using Creator = static_unique_ptr<screen_t> (*)(); //function pointer definition

    screen_t(window_t *parent = nullptr, win_type_t type = win_type_t::normal, is_closed_on_timeout_t timeout = is_closed_on_timeout_t::yes, is_closed_on_serial_t serial = is_closed_on_serial_t::yes)
        : IScreen(parent, type, timeout, serial) {}
};
