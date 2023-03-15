#pragma once

#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_tools.hpp"
#include "MItem_hardware.hpp"

using ScreenMenuHardwareChecks__ = ScreenMenu<GuiDefaults::MenuFooter,
    MI_RETURN,
    MI_NOZZLE_DIAMETER_CHECK,
    MI_PRINTER_MODEL_CHECK,
    MI_FIRMWARE_CHECK,
    MI_GCODE_LEVEL_CHECK>;

class ScreenMenuHardwareChecks : public ScreenMenuHardwareChecks__ {
public:
    constexpr static const char *label = N_("CHECKS");
    ScreenMenuHardwareChecks()
        : ScreenMenuHardwareChecks__(_(label)) {}
};
