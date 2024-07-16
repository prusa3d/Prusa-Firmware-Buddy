#pragma once

#include <screen_menu.hpp>
#include <window_menu_callback_item.hpp>

#include <MItem_menus.hpp>

using ScreenFilamentManagement_ = ScreenMenu<EFooter::Off,
    MI_RETURN,
    MI_EDIT_FILAMENTS //
    >;

class ScreenFilamentManagement final : public ScreenFilamentManagement_ {

public:
    ScreenFilamentManagement();
};
