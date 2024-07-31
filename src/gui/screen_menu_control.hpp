/**
 * @file screen_menu_control.hpp
 */
#pragma once

#include "screen_menu.hpp"
#include "MItem_menus.hpp"
#include "MItem_tools.hpp"
#include <option/filament_sensor.h>
#include <option/buddy_enable_connect.h>
#include <option/has_toolchanger.h>
#include <option/has_selftest.h>
#include <option/has_mmu2.h>
#include <option/has_coldpull.h>
#include <option/has_sheet_profiles.h>
#include <printers.h>
#include "MItem_basic_selftest.hpp"
#include "MItem_mmu.hpp"
#include <device/board.h>

#if XL_ENCLOSURE_SUPPORT()
    #include "MItem_enclosure.hpp"
#endif

#if HAS_SHEET_PROFILES()
    #include <MItem_steel_sheets.hpp>
#endif

using ScreenMenuControlSpec = ScreenMenu<EFooter::On, MI_RETURN,
#if HAS_TOOLCHANGER()
    MI_PICK_PARK_TOOL,
#endif
    MI_MOVE_AXIS,
    MI_TEMPERATURE,
    MI_AUTO_HOME,

#if BUDDY_ENABLE_CONNECT()
    MI_SET_READY,
#endif

#if HAS_SHEET_PROFILES()
    MI_CURRENT_SHEET_PROFILE,
#endif
    MI_DISABLE_STEP,
    MI_LIVE_ADJUST_Z,

#if XL_ENCLOSURE_SUPPORT()
    MI_ENCLOSURE_ENABLE,
    MI_ENCLOSURE,
#endif

#if HAS_COLDPULL()
    MI_COLD_PULL,
#endif

#if HAS_BELT_TUNING()
    MI_BELT_TUNING,
#endif

#if HAS_MMU2()
    MI_MMU_LOAD_TEST_FILAMENT,
#endif

#if PRINTER_IS_PRUSA_MK3_5() || PRINTER_IS_PRUSA_MINI()
    MI_MESH_BED,
    MI_BED_LEVEL_CORRECTION,
#endif
    MI_SELFTEST_SNAKE>;

class ScreenMenuControl : public ScreenMenuControlSpec {
public:
    constexpr static const char *label = N_("CONTROL");
    ScreenMenuControl();

protected:
    virtual void windowEvent(window_t *sender, GUI_event_t event, void *param) override;
};
