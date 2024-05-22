/**
 * @file screen_menu_mmu_load_filament.hpp
 */

#pragma once

#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_mmu.hpp"

template <typename>
struct ScreenMenuMMULoadTestFilament_;

template <size_t... i>
struct ScreenMenuMMULoadTestFilament_<std::index_sequence<i...>> {
    using T = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN, MI_MMU_LOAD_TEST_FILAMENT_I<i>...>;
};

class ScreenMenuMMULoadTestFilament : public ScreenMenuMMULoadTestFilament_<std::make_index_sequence<EXTRUDERS>>::T {
public:
    constexpr static const char *label = N_("Loading test");
    ScreenMenuMMULoadTestFilament();
};
