/// @file
#pragma once

#include <screen_menu.hpp>
#include <WindowMenuItems.hpp>
#include <MItem_tools.hpp>

using ScreenMenuWastebin_ = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN,
    MI_NOZZLE_CLEANER_FILL,
    MI_NOZZLE_CLEANER_EMPTY_WASTEBIN,
    MI_NOZZLE_CLEANER_CAPACITY,
    MI_NOZZLE_CLEANER_AUTOPAUSE,
#if HAS_INDX()
    MI_NOZZLE_CLEANER_X_OFFSET,
    MI_NOZZLE_CLEANER_Y_OFFSET,
#endif
    MI_ALWAYS_HIDDEN>;

class ScreenMenuWastebin : public ScreenMenuWastebin_ {
public:
    ScreenMenuWastebin()
        : ScreenMenuWastebin_(_("NOZZLE CLEANER")) {}
};
