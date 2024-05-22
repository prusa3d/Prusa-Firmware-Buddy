/**
 * @file screen_menu_mmu_eject_filament.hpp
 */

#pragma once

#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_mmu.hpp"

template <typename>
struct ScreenMenuMMUEjectFilament_;

template <size_t... i>
struct ScreenMenuMMUEjectFilament_<std::index_sequence<i...>> {
    using T = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN, MI_MMU_EJECT_FILAMENT_I<i>...>;
};

class ScreenMenuMMUEjectFilament : public ScreenMenuMMUEjectFilament_<std::make_index_sequence<EXTRUDERS>>::T {
public:
    constexpr static const char *label = N_("EJECT FROM MMU");
    ScreenMenuMMUEjectFilament();
};
