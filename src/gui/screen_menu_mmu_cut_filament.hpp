/**
 * @file screen_menu_mmu_cut_filament.hpp
 */

#pragma once

#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_mmu.hpp"

template <typename>
struct ScreenMenuMMUCutFilament_;

template <size_t... i>
struct ScreenMenuMMUCutFilament_<std::index_sequence<i...>> {
    using T = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN, MI_MMU_CUT_FILAMENT_I<i>...>;
};

class ScreenMenuMMUCutFilament : public ScreenMenuMMUCutFilament_<std::make_index_sequence<EXTRUDERS>>::T {
public:
    constexpr static const char *label = N_("Cut Filament");
    ScreenMenuMMUCutFilament();
};
