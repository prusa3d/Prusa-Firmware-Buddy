/// @file
#pragma once

#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_mmu.hpp"

template <typename>
struct ScreenMenuMMUPreloadToMMU_;

template <size_t... i>
struct ScreenMenuMMUPreloadToMMU_<std::index_sequence<i...>> {
    using T = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN, MI_MMU_PRELOAD_ALL, MI_MMU_PRELOAD_SLOT_I<i>...>;
};

class ScreenMenuMMUPreloadToMMU : public ScreenMenuMMUPreloadToMMU_<std::make_index_sequence<EXTRUDERS>>::T {
public:
    constexpr static const char *label = N_("Preload to MMU");
    ScreenMenuMMUPreloadToMMU();
};
