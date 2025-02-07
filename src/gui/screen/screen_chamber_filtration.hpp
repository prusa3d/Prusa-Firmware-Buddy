#pragma once

#include <gui/menu_item/specific/menu_items_chamber_filtration.hpp>
#include <screen_menu.hpp>

using ScreenChamberFiltration_ = ScreenMenu<EFooter::Off,
    MI_RETURN,
    MI_CHAMBER_FILTRATION_BACKEND,
    MI_CHAMBER_POST_PRINT_FILTRATION_DURATION,
    MI_CHAMBER_POST_PRINT_FILTRATION_POWER,
    MI_CHAMBER_PRINT_FILTRATION_POWER //
    >;

/// Management of a specified filament type
class ScreenChamberFiltration final : public ScreenChamberFiltration_ {
public:
    ScreenChamberFiltration();
};
