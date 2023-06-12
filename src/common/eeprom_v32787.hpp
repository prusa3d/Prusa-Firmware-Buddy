/**
 * @file eeprom_v32787.hpp
 * version 32787 from release 4.5.3 (first XL release)
 */

#include "eeprom_v12.hpp"
#include "printers.h"
#include "footer_eeprom.hpp"
#include "led_animations/led_types.h"
#include <option/development_items.h>

namespace eeprom::v32787 {

#pragma pack(push)
#pragma pack(1)

/**
 * @brief body of eeprom v32787
 * without head and crc
 */
struct vars_body_t : public eeprom::v12::vars_body_t {
    uint8_t TIME_FORMAT;
    float LOADCELL_SCALE;
    float LOADCELL_THRS_STATIC;
    float LOADCELL_HYST;
    float LOADCELL_THRS_CONTINOUS;
    int32_t FS_REF_VAL_0;
    uint32_t FS_VAL_SPAN_0;
    int32_t FS_REF_VALUE_1;
    uint32_t FS_VALUE_SPAN_1;
    int32_t FS_REF_VALUE_2;
    uint32_t FS_VALUE_SPAN_2;
    int32_t FS_REF_VALUE_3;
    uint32_t FS_VALUE_SPAN_3;
    int32_t FS_REF_VALUE_4;
    uint32_t FS_VALUE_SPAN_4;
    int32_t FS_REF_VALUE_5;
    uint32_t FS_VALUE_SPAN_5;
    int32_t EEVAR_SIDE_FS_REF_VALUE_0;
    uint32_t EEVAR_SIDE_FS_VALUE_SPAN_0;
    int32_t EEVAR_SIDE_FS_REF_VALUE_1;
    uint32_t EEVAR_SIDE_FS_VALUE_SPAN_1;
    int32_t EEVAR_SIDE_FS_REF_VALUE_2;
    uint32_t EEVAR_SIDE_FS_VALUE_SPAN_2;
    int32_t EEVAR_SIDE_FS_REF_VALUE_3;
    uint32_t EEVAR_SIDE_FS_VALUE_SPAN_3;
    int32_t EEVAR_SIDE_FS_REF_VALUE_4;
    uint32_t EEVAR_SIDE_FS_VALUE_SPAN_4;
    int32_t EEVAR_SIDE_FS_REF_VALUE_5;
    uint32_t EEVAR_SIDE_FS_VALUE_SPAN_5;
    uint16_t PRINT_PROGRESS_TIME;
    uint8_t TMC_WAVETABLE_ENABLE;
#if (EEPROM_FEATURES & EEPROM_FEATURE_MMU2)
    uint8_t EEVAR_MMU2_ENABLED;
    uint8_t EEVAR_MMU2_CUTTER;
    uint8_t EEVAR_MMU2_STEALTH_MODE;
#endif
    bool EEVAR_RUN_LEDS;
    Animation_model EEVAR_ANIMATION_IDLE;
    Animation_model EEVAR_ANIMATION_PRINTING;
    Animation_model EEVAR_ANIMATION_PAUSING;
    Animation_model EEVAR_ANIMATION_RESUMING;
    Animation_model EEVAR_ANIMATION_ABORTING;
    Animation_model EEVAR_ANIMATION_FINISHING;
    Animation_model EEVAR_ANIMATION_WARNING;
    Animation_model EEVAR_ANIMATION_POWER_PANIC;
    Animation_model EEVAR_ANIMATION_POWER_UP;
    Color_model EEVAR_ANIMATION_COLOR0;
    Color_model EEVAR_ANIMATION_COLOR1;
    Color_model EEVAR_ANIMATION_COLOR2;
    Color_model EEVAR_ANIMATION_COLOR3;
    Color_model EEVAR_ANIMATION_COLOR4;
    Color_model EEVAR_ANIMATION_COLOR5;
    Color_model EEVAR_ANIMATION_COLOR6;
    Color_model EEVAR_ANIMATION_COLOR7;
    Color_model EEVAR_ANIMATION_COLOR8;
    Color_model EEVAR_ANIMATION_COLOR9;
    Color_model EEVAR_ANIMATION_COLOR10;
    Color_model EEVAR_ANIMATION_COLOR11;
    Color_model EEVAR_ANIMATION_COLOR12;
    Color_model EEVAR_ANIMATION_COLOR13;
    Color_model EEVAR_ANIMATION_COLOR14;
    Color_model EEVAR_ANIMATION_COLOR_LAST;
    bool HEAT_ENTIRE_BED;
    bool TOUCH_ENABLED;
    DockPosition DOCK_POSITION_0;
    DockPosition DOCK_POSITION_1;
    DockPosition DOCK_POSITION_2;
    DockPosition DOCK_POSITION_3;
    DockPosition DOCK_POSITION_4;
    DockPosition DOCK_POSITION_5;
    ToolOffset TOOL_OFFSET_0;
    ToolOffset TOOL_OFFSET_1;
    ToolOffset TOOL_OFFSET_2;
    ToolOffset TOOL_OFFSET_3;
    ToolOffset TOOL_OFFSET_4;
    ToolOffset TOOL_OFFSET_5;
    uint8_t FILAMENT_TYPE_1;
    uint8_t FILAMENT_TYPE_2;
    uint8_t FILAMENT_TYPE_3;
    uint8_t FILAMENT_TYPE_4;
    uint8_t FILAMENT_TYPE_5;
    bool HEATUP_BED;
    float NOZZLE_DIA_0;
    float NOZZLE_DIA_1; // not used (reserved for XL)
    float NOZZLE_DIA_2; // not used (reserved for XL)
    float NOZZLE_DIA_3; // not used (reserved for XL)
    float NOZZLE_DIA_4; // not used (reserved for XL)
    float NOZZLE_DIA_5; // not used (reserved for XL)
    uint8_t EEVAR_HWCHECK_NOZZLE;
    uint8_t EEVAR_HWCHECK_MODEL;
    uint8_t EEVAR_HWCHECK_FIRMW;
    uint8_t EEVAR_HWCHECK_GCODE;
    SelftestResult_pre_23 SELFTEST_RESULT_PRE_23;
};

#pragma pack(pop)

constexpr vars_body_t body_defaults = {
    eeprom::v12::body_defaults,
    1,         // EEVAR_TIME_FORMAT
    0.0192,    // EEVAR_LOADCELL_SCALE
    -125,      // EEVAR_LOADCELL_THRS_STATIC
    80,        // EEVAR_LOADCELL_HYST
    -40,       // EEVAR_LOADCELL_THRS_CONTINOUS
    INT32_MIN, // EEVAR_FS_REF_VALUE_0, INT32_MIN == will require calibration
#if (BOARD_IS_XBUDDY && defined LOVEBOARD_HAS_PT100)
    100, // EEVAR_FS_VALUE_SPAN_0
#elif (BOARD_IS_XLBUDDY)
    1000,
#else
    350000, // EEVAR_FS_VALUE_SPAN_0
#endif
    INT32_MIN, // EEVAR_FS_REF_VALUE_1, INT32_MIN == will require calibration
    1000,      // FS_VALUE_SPAN_1
    INT32_MIN, // EEVAR_FS_REF_VALUE_2, INT32_MIN == will require calibration
    1000,      // FS_VALUE_SPAN_2
    INT32_MIN, // EEVAR_FS_REF_VALUE_3, INT32_MIN == will require calibration
    1000,      // FS_VALUE_SPAN_3
    INT32_MIN, // EEVAR_FS_REF_VALUE_4, INT32_MIN == will require calibration
    1000,      // FS_VALUE_SPAN_4
    INT32_MIN, // EEVAR_FS_REF_VALUE_5, INT32_MIN == will require calibration
    1000,      // FS_VALUE_SPAN_5
    INT32_MIN, // EEVAR_SIDE_FS_REF_VALUE_0, INT32_MIN == will require calibration
    310,       // EEVAR_SIDE_FS_VALUE_SPAN_0
    INT32_MIN, // EEVAR_SIDE_FS_REF_VALUE_1, INT32_MIN == will require calibration
    310,       // EEVAR_SIDE_FS_VALUE_SPAN_1
    INT32_MIN, // EEVAR_SIDE_FS_REF_VALUE_2, INT32_MIN == will require calibration
    310,       // EEVAR_SIDE_FS_VALUE_SPAN_2
    INT32_MIN, // EEVAR_SIDE_FS_REF_VALUE_3, INT32_MIN == will require calibration
    310,       // EEVAR_SIDE_FS_VALUE_SPAN_3
    INT32_MIN, // EEVAR_SIDE_FS_REF_VALUE_4, INT32_MIN == will require calibration
    310,       // EEVAR_SIDE_FS_VALUE_SPAN_4
    INT32_MIN, // EEVAR_SIDE_FS_REF_VALUE_5, INT32_MIN == will require calibration
    310,       // EEVAR_SIDE_FS_VALUE_SPAN_5
    30,        // EEVAR_PRINT_PROGRESS_TIME
    true,      // EEVAR_TMC_WAVETABLE_ENABLED
#if (EEPROM_FEATURES & EEPROM_FEATURE_MMU2)
    false, // EEVAR_MMU2_ENABLED
    false, // EEVAR_MMU2_CUTTER
    false, // EEVAR_MMU2_STEALTH_MODE
#endif
    true,                                                                       // EEVAR_RUN_LEDS
    { static_cast<uint8_t>(AnimationTypes::SolidColor), 0, 0, 0, 0, 0, 0 },     // EEVAR_ANIMATION_IDLE
    { static_cast<uint8_t>(AnimationTypes::SolidColor), 0, 150, 255, 0, 0, 0 }, // EEVAR_ANIMATION_PRINTING
    { static_cast<uint8_t>(AnimationTypes::SolidColor), 0, 150, 255, 0, 0, 0 }, // EEVAR_ANIMATION_PAUSING
    { static_cast<uint8_t>(AnimationTypes::SolidColor), 0, 150, 255, 0, 0, 0 }, // EEVAR_ANIMATION_RESUMING
    { static_cast<uint8_t>(AnimationTypes::SolidColor), 0, 0, 0, 0, 0, 0 },     // EEVAR_ANIMATION_ABORTING
    { static_cast<uint8_t>(AnimationTypes::SolidColor), 0, 255, 0, 0, 0, 0 },   // EEVAR_ANIMATION_FINISHING
    { static_cast<uint8_t>(AnimationTypes::Fading), 255, 255, 0, 1000, 0, 0 },  // EEVAR_ANIMATION_WARNING
    { static_cast<uint8_t>(AnimationTypes::SolidColor), 0, 0, 0, 0, 0, 0 },     // EEVAR_ANIMATION_POWER_PANIC
    { static_cast<uint8_t>(AnimationTypes::Fading), 0, 255, 0, 1500, 0, 0 },    // EEVAR_ANIMATION_POWER_UP
    { 0, 0, 0, 0 },                                                             // EEVAR_ANIMATION_COLOR0
    { 0, 0, 0, 0 },                                                             // EEVAR_ANIMATION_COLOR1
    { 0, 0, 0, 0 },                                                             // EEVAR_ANIMATION_COLOR2
    { 0, 0, 0, 0 },                                                             // EEVAR_ANIMATION_COLOR3
    { 0, 0, 0, 0 },                                                             // EEVAR_ANIMATION_COLOR4
    { 0, 0, 0, 0 },                                                             // EEVAR_ANIMATION_COLOR5
    { 0, 0, 0, 0 },                                                             // EEVAR_ANIMATION_COLOR6
    { 0, 0, 0, 0 },                                                             // EEVAR_ANIMATION_COLOR7
    { 0, 0, 0, 0 },                                                             // EEVAR_ANIMATION_COLOR8
    { 0, 0, 0, 0 },                                                             // EEVAR_ANIMATION_COLOR9
    { 0, 0, 0, 0 },                                                             // EEVAR_ANIMATION_COLOR10
    { 0, 0, 0, 0 },                                                             // EEVAR_ANIMATION_COLOR11
    { 0, 0, 0, 0 },                                                             // EEVAR_ANIMATION_COLOR12
    { 0, 0, 0, 0 },                                                             // EEVAR_ANIMATION_COLOR13
    { 0, 0, 0, 0 },                                                             // EEVAR_ANIMATION_COLOR14
    { 0, 0, 0, 0 },                                                             // EEVAR_ANIMATION_COLOR_LAST
    false,                                                                      // EEVAR_HEAT_ENTIRE_BED
    option::development_items,                                                  // EEVAR_TOUCH_ENABLED
    { 0, 0 },                                                                   // EEVAR_DOCK_POSITION_0
    { 0, 0 },                                                                   // EEVAR_DOCK_POSITION_1
    { 0, 0 },                                                                   // EEVAR_DOCK_POSITION_2
    { 0, 0 },                                                                   // EEVAR_DOCK_POSITION_3
    { 0, 0 },                                                                   // EEVAR_DOCK_POSITION_4
    { 0, 0 },                                                                   // EEVAR_DOCK_POSITION_5
    { 0, 0, 0 },                                                                // EEVAR_TOOL_OFFSET_0
    { 0, 0, 0 },                                                                // EEVAR_TOOL_OFFSET_1
    { 0, 0, 0 },                                                                // EEVAR_TOOL_OFFSET_2
    { 0, 0, 0 },                                                                // EEVAR_TOOL_OFFSET_3
    { 0, 0, 0 },                                                                // EEVAR_TOOL_OFFSET_4
    { 0, 0, 0 },                                                                // EEVAR_TOOL_OFFSET_5
    0,                                                                          // EEVAR_FILAMENT_TYPE_1
    0,                                                                          // EEVAR_FILAMENT_TYPE_2
    0,                                                                          // EEVAR_FILAMENT_TYPE_3
    0,                                                                          // EEVAR_FILAMENT_TYPE_4
    0,                                                                          // EEVAR_FILAMENT_TYPE_5
    false,                                                                      // EEVAR_HEATUP_BED
#if PRINTER_TYPE == PRINTER_PRUSA_XL
    0.60f, // EEVAR_NOZZLE_DIA_0
    0.60f, // EEVAR_NOZZLE_DIA_1
    0.60f, // EEVAR_NOZZLE_DIA_2
    0.60f, // EEVAR_NOZZLE_DIA_3
    0.60f, // EEVAR_NOZZLE_DIA_4
    0.60f, // EEVAR_NOZZLE_DIA_5
#else
    0.40f, // EEVAR_NOZZLE_DIA_0
    0.40f, // EEVAR_NOZZLE_DIA_1
    0.40f, // EEVAR_NOZZLE_DIA_2
    0.40f, // EEVAR_NOZZLE_DIA_3
    0.40f, // EEVAR_NOZZLE_DIA_4
    0.40f, // EEVAR_NOZZLE_DIA_5
#endif
    1,  // EEVAR_HWCHECK_NOZZLE
    1,  // EEVAR_HWCHECK_MODEL
    1,  // EEVAR_HWCHECK_FIRMW
    1,  // EEVAR_HWCHECK_GCODE
    {}, // EEVAR_SELFTEST_RESULT_PRE_23
};

inline vars_body_t convert(const eeprom::v12::vars_body_t &src) {
    vars_body_t ret = body_defaults;

    // copy entire v12 struct
    memcpy(&ret, &src, sizeof(eeprom::v12::vars_body_t));

    return ret;
}

} // namespace
