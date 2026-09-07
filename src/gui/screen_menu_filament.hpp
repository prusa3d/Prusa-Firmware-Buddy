/**
 * @file screen_menu_filament.hpp
 */
#pragma once

#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_filament.hpp"
#include "MItem_menus.hpp"
#include "MItem_tools.hpp"
#include <option/has_toolchanger.h>
#include <option/has_wastebin_fill_tracking.h>
#include <gui/screen/filament/screen_filaments_loaded.hpp>

using ScreenMenuFilament__ = ScreenMenu<GuiDefaults::MenuFooter,
    MI_RETURN,
    MI_LOADED_FILAMENT,
#if HAS_WASTEBIN_FILL_TRACKING()
    MI_NOZZLE_CLEANER_EMPTY_WASTEBIN,
#endif
#if HAS_TOOLCHANGER()
    MI_LOAD_ALL,
#endif
    MI_LOAD,
#if HAS_TOOLCHANGER()
    MI_UNLOAD_ALL,
#endif
    MI_UNLOAD,
#if HAS_TOOLCHANGER()
    MI_CHANGEALL,
#endif
    MI_CHANGE,
    MI_PURGE,
    MI_FILAMENT_MANAGEMENT //
    >;

class ScreenMenuFilament : public ScreenMenuFilament__ {
public:
    constexpr static const char *label = N_("FILAMENT");
    ScreenMenuFilament();
};
