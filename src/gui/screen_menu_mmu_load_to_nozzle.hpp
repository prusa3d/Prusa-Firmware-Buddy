/**
 * @file screen_menu_mmu_load_to_nozzle.hpp
 */

#pragma once

#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_mmu.hpp"

template <typename>
struct ScreenMenuMMULoadToNozzle_;

template <size_t... i>
struct ScreenMenuMMULoadToNozzle_<std::index_sequence<i...>> {
    using T = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN, MI_MMU_LOAD_TO_NOZZLE_I<i>...>;
};

class ScreenMenuMMULoadToNozzle : public ScreenMenuMMULoadToNozzle_<std::make_index_sequence<EXTRUDERS>>::T {
public:
    constexpr static const char *label = N_("Load to Nozzle");
    ScreenMenuMMULoadToNozzle();
};
