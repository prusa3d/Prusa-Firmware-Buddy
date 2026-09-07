/// @file
#pragma once

#include <basic_screen_menu.hpp>
#include "MItem_hardware.hpp"

template <typename>
struct ScreenMenuGcodeChecks_;

template <size_t... ix>
struct ScreenMenuGcodeChecks_<std::index_sequence<ix...>> {
    using T = BasicScreenMenu<
        WithConstructorArgs<MI_HARDWARE_CHECK, static_cast<HWCheckType>(ix)>...>;
};

class ScreenMenuGcodeChecks : public ScreenMenuGcodeChecks_<std::make_index_sequence<hw_check_type_count>>::T {
public:
    ScreenMenuGcodeChecks()
        : BasicScreenMenu(_("G-CODE CHECKS")) {}
};
