/**
 * @file screen_menu_version_info_non_mini.hpp
 */

#pragma once

#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#if PRINTER_IS_PRUSA_MK4 || PRINTER_IS_PRUSA_iX
    #include "MItem_love_board.hpp"
#endif
#include "menu_item_xlcd.hpp"
#include "MItem_tools.hpp"
#include <guiconfig/GuiDefaults.hpp>
#include "printers.h"

using ScreenMenuVersionInfo__ = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN, MI_INFO_FW, MI_INFO_BOOTLOADER, MI_INFO_MMU, MI_INFO_BOARD, MI_INFO_SERIAL_NUM,
#if PRINTER_IS_PRUSA_MK4 || PRINTER_IS_PRUSA_iX
    MI_INFO_SERIAL_NUM_LOVEBOARD,
#endif
    MI_INFO_SERIAL_NUM_XLCD,
#if PRINTER_IS_PRUSA_MK4 || PRINTER_IS_PRUSA_iX
    MI_LOVEBOARD_SINGLE_ERR,
    MI_LOVEBOARD_REPEATED_ERR,
    MI_LOVEBOARD_CYCLIC_ERR,
    MI_LOVEBOARD_RETRIED,
    MI_LOVEBOARD_STATUS,
#endif
    MI_XLCD_SINGLE_ERR,
    MI_XLCD_REPEATED_ERR,
    MI_XLCD_CYCLIC_ERR,
    MI_XLCD_RETRIED,
    MI_XLCD_STATUS>;

class ScreenMenuVersionInfo : public ScreenMenuVersionInfo__ {
    void set_serial_number(WiInfo<28> &item, const char *sn, uint8_t bom_id);

public:
    constexpr static const char *label = N_("VERSION INFO");
    ScreenMenuVersionInfo();
};
