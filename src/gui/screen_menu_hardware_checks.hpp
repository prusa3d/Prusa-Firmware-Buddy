#pragma once

#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_hardware.hpp"

template <typename>
struct ScreenMenuHardwareChecks_;

template <size_t... ix>
struct ScreenMenuHardwareChecks_<std::index_sequence<ix...>> {
    using T = ScreenMenu<GuiDefaults::MenuFooter,
        MI_RETURN,
        WithConstructorArgs<MI_HARDWARE_CHECK, static_cast<HWCheckType>(ix)>...>;
};

class ScreenMenuHardwareChecks : public ScreenMenuHardwareChecks_<std::make_index_sequence<hw_check_type_count>>::T {
public:
    ScreenMenuHardwareChecks()
        : ScreenMenu(_("CHECKS")) {}
};
