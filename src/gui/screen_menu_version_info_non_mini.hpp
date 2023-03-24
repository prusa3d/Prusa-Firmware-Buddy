/**
 * @file screen_menu_version_info_non_mini.hpp
 */

#pragma once

#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_tools.hpp"
#include "GuiDefaults.hpp"
#include "printers.h"

using ScreenMenuVersionInfo__ = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN, MI_INFO_FW, MI_INFO_BOOTLOADER, MI_INFO_BOARD, MI_INFO_SERIAL_NUM,
#if PRINTER_TYPE == PRINTER_PRUSA_MK4
    MI_INFO_SERIAL_NUM_LOVEBOARD,
#endif
    MI_INFO_SERIAL_NUM_XLCD>;

class ScreenMenuVersionInfo : public ScreenMenuVersionInfo__ {
    void set_serial_number(WiInfo<28> &item, const char *sn, uint8_t bom_id);

public:
    constexpr static const char *label = N_("VERSION INFO");
    ScreenMenuVersionInfo();
};
