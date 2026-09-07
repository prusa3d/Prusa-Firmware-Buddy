/// @file
#pragma once

#include <gui/basic_screen_menu.hpp>
#include <gui/menu_item/specific/menu_items_chamber_filtration.hpp>

using ScreenChamberFiltrationBase = BasicScreenMenu<
    MI_CHAMBER_FILTRATION_BACKEND,
    MI_CHAMBER_FILTER_TIME_USED,
    MI_CHAMBER_CHANGE_FILTER,
    MI_CHAMBER_PRINT_FILTRATION,
    MI_CHAMBER_PRINT_FILTRATION_POWER,
    MI_CHAMBER_POST_PRINT_FILTRATION,
    MI_CHAMBER_POST_PRINT_FILTRATION_DURATION,
    MI_CHAMBER_POST_PRINT_FILTRATION_POWER,
    MI_CHAMBER_ALWAYS_FILTER>;

class ScreenChamberFiltration final : public ScreenChamberFiltrationBase {
public:
    ScreenChamberFiltration();
};
