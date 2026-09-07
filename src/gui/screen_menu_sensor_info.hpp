#pragma once

#include <option/has_door_sensor.h>
#include <option/has_mmu2.h>
#include <option/has_loadcell.h>
#include <option/has_toolchanger.h>
#include <option/has_remote_bed.h>
#include <option/has_chamber_api.h>
#include <option/has_per_tool_temperatures.h>
#include <option/has_extruder_fsensor.h>
#include <option/has_side_fsensor.h>

#include <Configuration_adv.h>
#include <fs_autoload_autolock.hpp>
#include <gui/menu_item/menu_item_virtual_submenu.hpp>

#include <basic_screen_menu.hpp>
#include <MItem_tools.hpp>

#include <MItem_menus.hpp>
#include <MItem_print.hpp>

#if HAS_MMU2()
    #include <MItem_mmu.hpp>
#endif
#if HAS_LOADCELL()
    #include <MItem_loadcell.hpp>
#endif
#if HAS_REMOTE_BED()
    #include "screen_menu_remote_bed.hpp"
#endif
#if HAS_CHAMBER_API()
    #include <gui/menu_item/specific/menu_items_chamber.hpp>
#endif

#if PRINTER_IS_PRUSA_MK3_5()
    #include <MItem_MK3.5.hpp>
#endif
#if PRINTER_IS_PRUSA_MINI()
    #include <MItem_MINI.hpp>
#endif

#if HAS_DWARF() || HAS_INDX()

class MI_INFO_HEAD_PCB_TEMPERATURE : public MenuItemAutoUpdatingLabel<float> {
public:
    MI_INFO_HEAD_PCB_TEMPERATURE();
};

class MI_INFO_HEAD_MCU_TEMPERATURE : public MenuItemAutoUpdatingLabel<float> {
public:
    MI_INFO_HEAD_MCU_TEMPERATURE();
};

#endif

#if HAS_INDX()

class MI_INFO_HEAD_AMBIENT_TEMPERATURE : public MenuItemAutoUpdatingLabel<float> {
public:
    MI_INFO_HEAD_AMBIENT_TEMPERATURE();
};

class MI_INFO_NOZZLE_TEMP_UNCOMPENSATED : public MenuItemAutoUpdatingLabel<float> {
public:
    MI_INFO_NOZZLE_TEMP_UNCOMPENSATED();
};

class MI_INFO_NOZZLE_POWER : public MenuItemAutoUpdatingLabel<float> {
public:
    MI_INFO_NOZZLE_POWER();
};

class MI_INFO_RINGDOWN_DECAY : public MenuItemAutoUpdatingLabel<int16_t> {
public:
    MI_INFO_RINGDOWN_DECAY();
};

#endif

using ScreenMenuSensorInfo_ = BasicScreenMenu<
#if PRINTER_IS_PRUSA_MINI()
    // Take very minimalist approach for the Mini, we're low on FLASH right now :(
    // TODO: Remove this
    MI_INFO_EXTRUDER_FILAMENT_SENSOR,
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
    MI_INFO_NOZZLE_TEMP,
    #if HAS_PER_TOOL_TEMPERATURES()
    MenuItemVirtualSubmenu<N_("Nozzle Temperatures"), MI_INFO_NOZZLE_TEMP, PhysicalToolIndex::count, PhysicalToolIndex::from_raw>,
    #endif
    #if HAS_TEMP_HEATBREAK
    MI_INFO_HEATBREAK_TEMP,
        #if HAS_PER_TOOL_TEMPERATURES()
    MenuItemVirtualSubmenu<N_("Heatbreak Temperatures"), MI_INFO_HEATBREAK_TEMP, PhysicalToolIndex::count, PhysicalToolIndex::from_raw>,
        #endif
    #endif
    #if HAS_DWARF() || HAS_INDX()
    MI_INFO_HEAD_PCB_TEMPERATURE,
    MI_INFO_HEAD_MCU_TEMPERATURE,
    #endif
    #if HAS_INDX()
    MI_INFO_HEAD_AMBIENT_TEMPERATURE,
    MI_INFO_NOZZLE_TEMP_UNCOMPENSATED,
    MI_INFO_NOZZLE_POWER,
    MI_INFO_RINGDOWN_DECAY,
    #endif
    #if HAS_REMOTE_BED()
    MI_INFO_REMOTE_BED_MCU_TEMPERATURE,
    #endif

    #if HAS_LOADCELL()
    MI_INFO_LOADCELL,
    #endif
    #if HAS_DOOR_SENSOR()
    MI_INFO_DOOR_SENSOR,
    #endif
    #if HAS_EXTRUDER_FSENSOR()
    MI_INFO_EXTRUDER_FILAMENT_SENSOR,
        #if HOTENDS > 1
    MenuItemVirtualSubmenu<N_("Extruder Filament Sensors"), MI_INFO_EXTRUDER_FILAMENT_SENSOR, PhysicalToolIndex::count, PhysicalToolIndex::from_raw>,
        #endif
    #endif // HAS_EXTRUDER_FSENSOR()
    #if HAS_SIDE_FSENSOR()
    MI_INFO_SIDE_FILAMENT_SENSOR,
        #if HOTENDS > 1
    MenuItemVirtualSubmenu<N_("Side Filament Sensors"), MI_INFO_SIDE_FILAMENT_SENSOR, PhysicalToolIndex::count, PhysicalToolIndex::from_raw>,
        #endif
    #endif // HAS_SIDE_FSENSOR()
    #if PRINTER_IS_PRUSA_MK3_5()
    MI_PINDA,
    #endif
    #if PRINTER_IS_PRUSA_MINI()
    // #error dead code found by automatic analyses (see BFW-5461)
    MI_MINDA,
    #endif
    #if HAS_MMU2()
    MI_INFO_FINDA,
    #endif

    #if BOARD_IS_XBUDDY()
    MI_INFO_BED_VOLTAGE,
        #if !HAS_INDX() // INDX head circumvents this
    MI_INFO_HEATER_VOLTAGE,
    MI_INFO_HEATER_CURRENT,
        #endif
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
    MI_FAN_INFO
#endif
    >;

class ScreenMenuSensorInfo final : public ScreenMenuSensorInfo_ {
    FS_AutoloadAutolock lock;

public:
    ScreenMenuSensorInfo();
};
