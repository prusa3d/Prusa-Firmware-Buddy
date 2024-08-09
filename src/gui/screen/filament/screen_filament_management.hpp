#pragma once

#include <screen_menu.hpp>
#include <window_menu_callback_item.hpp>

#include <MItem_menus.hpp>

#include "screen_filaments_loaded.hpp"

using ScreenFilamentManagement_ = ScreenMenu<EFooter::Off,
    MI_RETURN,
    MI_LOADED_FILAMENT,
    MI_EDIT_FILAMENTS,
    MI_REORDER_FILAMENTS,
    MI_FILAMENTS_VISIBILITY //
    >;

class ScreenFilamentManagement final : public ScreenFilamentManagement_ {

public:
    ScreenFilamentManagement();
};
