#pragma once

#include <option/has_mmu2.h>
#include <option/has_loadcell.h>
#include <option/has_toolchanger.h>
#include <option/has_modularbed.h>
#include <option/has_chamber_api.h>
#include <option/has_xbuddy_extension.h>

#include <Configuration_adv.h>
#include <fs_autoload_autolock.hpp>

#include <screen_menu.hpp>
#include <MItem_tools.hpp>

#include <MItem_love_board.hpp>
#include <MItem_print.hpp>

#if HAS_MMU2()
    #include <MItem_mmu.hpp>
#endif
#if HAS_LOADCELL()
    #include <MItem_loadcell.hpp>
#endif
#if HAS_TOOLCHANGER()
    #include <screen_menu_tools.hpp>
#endif
#if HAS_MODULARBED()
    #include <screen_menu_modularbed.hpp>
#endif
#if HAS_CHAMBER_API()
    #include <gui/menu_item/specific/menu_items_chamber.hpp>
#endif
#if HAS_XBUDDY_EXTENSION()
    #include <gui/menu_item/specific/menu_items_xbuddy_extension.hpp>
#endif

#if PRINTER_IS_PRUSA_MK3_5()
    #include <MItem_MK3.5.hpp>
#endif
#if PRINTER_IS_PRUSA_MINI()
    #include <MItem_MINI.hpp>
#endif

template <typename>
struct ScreenMenuSensorInfo__;

template <size_t... hotend>
struct ScreenMenuSensorInfo__<std::index_sequence<hotend...>> {
    using T = ScreenMenu<GuiDefaults::MenuFooter,
        MI_RETURN,

#if PRINTER_IS_PRUSA_MINI()
        // Take very minimalist approach for the Mini, we're low on FLASH right now :(
        // TODO: Remove this
        MI_INFO_PRINTER_FILL_SENSOR,
        MI_MINDA,
        MI_INFO_MCU_TEMP

#else

    #if HAS_TEMP_BOARD
        MI_INFO_BOARD_TEMP,
    #endif
        MI_INFO_MCU_TEMP,
        MI_INFO_BED_TEMP,
    #if HAS_CHAMBER_API()
        MI_CHAMBER_TEMP,
    #endif
        WithConstructorArgs<MI_INFO_NOZZLE_TEMP, hotend>...,
    #if TEMP_SENSOR_HEATBREAK > 0
        WithConstructorArgs<MI_INFO_HEATBREAK_TEMP, hotend>...,
    #endif
    #if HAS_TOOLCHANGER()
        MI_INFO_DWARF_BOARD_TEMPERATURE,
        MI_INFO_DWARF_MCU_TEMPERATURE,
    #endif
    #if HAS_MODULARBED()
        MI_INFO_MODULAR_BED_MCU_TEMPERATURE,
    #endif

    #if HAS_LOADCELL()
        MI_INFO_LOADCELL,
    #endif
        MI_INFO_PRINTER_FILL_SENSOR,
        MI_INFO_SIDE_FILL_SENSOR,
    #if PRINTER_IS_PRUSA_MK3_5()
        MI_PINDA,
    #endif
    #if PRINTER_IS_PRUSA_MINI()
        MI_MINDA,
    #endif
    #if HAS_MMU2()
        MI_INFO_FINDA,
    #endif

        MI_INFO_PRINT_FAN,
        MI_INFO_HBR_FAN,
    #if HAS_XBUDDY_EXTENSION()
        MI_INFO_XBUDDY_EXTENSION_FAN1,
        MI_INFO_XBUDDY_EXTENSION_FAN2,
        MI_INFO_XBUDDY_EXTENSION_FAN3,
    #endif

    #if BOARD_IS_XBUDDY()
        MI_INFO_HEATER_VOLTAGE,
        MI_INFO_INPUT_VOLTAGE,
        MI_INFO_HEATER_CURRENT,
        MI_INFO_INPUT_CURRENT,
    #endif
    #if BOARD_IS_XLBUDDY()
        MI_INFO_5V_VOLTAGE,
        MI_INFO_SANDWICH_5V_CURRENT,
        MI_INFO_BUDDY_5V_CURRENT,
    #endif
    #if HAS_MMU2()
        MI_INFO_MMU_CURRENT,
    #endif
        MI_ALWAYS_HIDDEN
#endif
        >;
};

using ScreenMenuSensorInfo_ = ScreenMenuSensorInfo__<std::make_index_sequence<HOTENDS>>::T;

class ScreenMenuSensorInfo : public ScreenMenuSensorInfo_ {
    FS_AutoloadAutolock lock;

protected:
    virtual void windowEvent(window_t *sender, GUI_event_t event, void *param) override;

public:
    ScreenMenuSensorInfo();
};
