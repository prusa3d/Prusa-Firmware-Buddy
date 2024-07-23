/**
 * @file eeprom_v32787.hpp
 * version 32787 from release 4.5.3 (first XL release)
 */

#pragma once
#include "eeprom_v12.hpp"
#include "printers.h"
#include "footer_eeprom.hpp"
#include "led_animations/led_types.h"
#include <option/development_items.h>
#include "selftest_result.hpp"
#include <gui/led_animations/animation_model.hpp>
#include <module/prusa/dock_position.hpp>
#include <module/prusa/tool_offset.hpp>

namespace config_store_ns::old_eeprom::v32787 {

#pragma pack(push)
#pragma pack(1)

/**
 * @brief Unused & dropped
 *
 */
struct Color_model {
    uint8_t R;
    uint8_t G;
    uint8_t B;
    uint8_t next_index;
};

/**
 * @brief body of eeprom v32787
 * without head and crc
 */
struct vars_body_t : public old_eeprom::v12::vars_body_t {
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
    uint8_t EEVAR_MMU2_ENABLED;
    uint8_t EEVAR_MMU2_CUTTER;
    uint8_t EEVAR_MMU2_STEALTH_MODE;
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
    old_eeprom::v12::body_defaults,
    1, // EEVAR_TIME_FORMAT
    0.0192, // EEVAR_LOADCELL_SCALE
    -125, // EEVAR_LOADCELL_THRS_STATIC
    80, // EEVAR_LOADCELL_HYST
    -40, // EEVAR_LOADCELL_THRS_CONTINOUS
    std::numeric_limits<int32_t>::min(), // EEVAR_FS_REF_VALUE_0, std::numeric_limits<int32_t>::min() == will require calibration
#if (BOARD_IS_XBUDDY && defined LOVEBOARD_HAS_PT100)
    100, // EEVAR_FS_VALUE_SPAN_0
#elif (BOARD_IS_XLBUDDY)
    1000,
#else
    350000, // EEVAR_FS_VALUE_SPAN_0
#endif
    std::numeric_limits<int32_t>::min(), // EEVAR_FS_REF_VALUE_1, std::numeric_limits<int32_t>::min() == will require calibration
    1000, // FS_VALUE_SPAN_1
    std::numeric_limits<int32_t>::min(), // EEVAR_FS_REF_VALUE_2, std::numeric_limits<int32_t>::min() == will require calibration
    1000, // FS_VALUE_SPAN_2
    std::numeric_limits<int32_t>::min(), // EEVAR_FS_REF_VALUE_3, std::numeric_limits<int32_t>::min() == will require calibration
    1000, // FS_VALUE_SPAN_3
    std::numeric_limits<int32_t>::min(), // EEVAR_FS_REF_VALUE_4, std::numeric_limits<int32_t>::min() == will require calibration
    1000, // FS_VALUE_SPAN_4
    std::numeric_limits<int32_t>::min(), // EEVAR_FS_REF_VALUE_5, std::numeric_limits<int32_t>::min() == will require calibration
    1000, // FS_VALUE_SPAN_5
    std::numeric_limits<int32_t>::min(), // EEVAR_SIDE_FS_REF_VALUE_0, std::numeric_limits<int32_t>::min() == will require calibration
    310, // EEVAR_SIDE_FS_VALUE_SPAN_0
    std::numeric_limits<int32_t>::min(), // EEVAR_SIDE_FS_REF_VALUE_1, std::numeric_limits<int32_t>::min() == will require calibration
    310, // EEVAR_SIDE_FS_VALUE_SPAN_1
    std::numeric_limits<int32_t>::min(), // EEVAR_SIDE_FS_REF_VALUE_2, std::numeric_limits<int32_t>::min() == will require calibration
    310, // EEVAR_SIDE_FS_VALUE_SPAN_2
    std::numeric_limits<int32_t>::min(), // EEVAR_SIDE_FS_REF_VALUE_3, std::numeric_limits<int32_t>::min() == will require calibration
    310, // EEVAR_SIDE_FS_VALUE_SPAN_3
    std::numeric_limits<int32_t>::min(), // EEVAR_SIDE_FS_REF_VALUE_4, std::numeric_limits<int32_t>::min() == will require calibration
    310, // EEVAR_SIDE_FS_VALUE_SPAN_4
    std::numeric_limits<int32_t>::min(), // EEVAR_SIDE_FS_REF_VALUE_5, std::numeric_limits<int32_t>::min() == will require calibration
    310, // EEVAR_SIDE_FS_VALUE_SPAN_5
    30, // EEVAR_PRINT_PROGRESS_TIME
    true, // EEVAR_TMC_WAVETABLE_ENABLED
    false, // EEVAR_MMU2_ENABLED
    false, // EEVAR_MMU2_CUTTER
    false, // EEVAR_MMU2_STEALTH_MODE
    true, // EEVAR_RUN_LEDS
    { static_cast<uint8_t>(AnimationTypes::SolidColor), 0, 0, 0, 0, 0, 0 }, // EEVAR_ANIMATION_IDLE
    { static_cast<uint8_t>(AnimationTypes::SolidColor), 0, 150, 255, 0, 0, 0 }, // EEVAR_ANIMATION_PRINTING
    { static_cast<uint8_t>(AnimationTypes::SolidColor), 0, 150, 255, 0, 0, 0 }, // EEVAR_ANIMATION_PAUSING
    { static_cast<uint8_t>(AnimationTypes::SolidColor), 0, 150, 255, 0, 0, 0 }, // EEVAR_ANIMATION_RESUMING
    { static_cast<uint8_t>(AnimationTypes::SolidColor), 0, 0, 0, 0, 0, 0 }, // EEVAR_ANIMATION_ABORTING
    { static_cast<uint8_t>(AnimationTypes::SolidColor), 0, 255, 0, 0, 0, 0 }, // EEVAR_ANIMATION_FINISHING
    { static_cast<uint8_t>(AnimationTypes::Fading), 255, 255, 0, 1000, 0, 0 }, // EEVAR_ANIMATION_WARNING
    { static_cast<uint8_t>(AnimationTypes::SolidColor), 0, 0, 0, 0, 0, 0 }, // EEVAR_ANIMATION_POWER_PANIC
    { static_cast<uint8_t>(AnimationTypes::Fading), 0, 255, 0, 1500, 0, 0 }, // EEVAR_ANIMATION_POWER_UP
    { 0, 0, 0, 0 }, // EEVAR_ANIMATION_COLOR0
    { 0, 0, 0, 0 }, // EEVAR_ANIMATION_COLOR1
    { 0, 0, 0, 0 }, // EEVAR_ANIMATION_COLOR2
    { 0, 0, 0, 0 }, // EEVAR_ANIMATION_COLOR3
    { 0, 0, 0, 0 }, // EEVAR_ANIMATION_COLOR4
    { 0, 0, 0, 0 }, // EEVAR_ANIMATION_COLOR5
    { 0, 0, 0, 0 }, // EEVAR_ANIMATION_COLOR6
    { 0, 0, 0, 0 }, // EEVAR_ANIMATION_COLOR7
    { 0, 0, 0, 0 }, // EEVAR_ANIMATION_COLOR8
    { 0, 0, 0, 0 }, // EEVAR_ANIMATION_COLOR9
    { 0, 0, 0, 0 }, // EEVAR_ANIMATION_COLOR10
    { 0, 0, 0, 0 }, // EEVAR_ANIMATION_COLOR11
    { 0, 0, 0, 0 }, // EEVAR_ANIMATION_COLOR12
    { 0, 0, 0, 0 }, // EEVAR_ANIMATION_COLOR13
    { 0, 0, 0, 0 }, // EEVAR_ANIMATION_COLOR14
    { 0, 0, 0, 0 }, // EEVAR_ANIMATION_COLOR_LAST
    false, // EEVAR_HEAT_ENTIRE_BED
    option::development_items, // EEVAR_TOUCH_ENABLED
    { 0, 0 }, // EEVAR_DOCK_POSITION_0
    { 0, 0 }, // EEVAR_DOCK_POSITION_1
    { 0, 0 }, // EEVAR_DOCK_POSITION_2
    { 0, 0 }, // EEVAR_DOCK_POSITION_3
    { 0, 0 }, // EEVAR_DOCK_POSITION_4
    { 0, 0 }, // EEVAR_DOCK_POSITION_5
    { 0, 0, 0 }, // EEVAR_TOOL_OFFSET_0
    { 0, 0, 0 }, // EEVAR_TOOL_OFFSET_1
    { 0, 0, 0 }, // EEVAR_TOOL_OFFSET_2
    { 0, 0, 0 }, // EEVAR_TOOL_OFFSET_3
    { 0, 0, 0 }, // EEVAR_TOOL_OFFSET_4
    { 0, 0, 0 }, // EEVAR_TOOL_OFFSET_5
    0, // EEVAR_FILAMENT_TYPE_1
    0, // EEVAR_FILAMENT_TYPE_2
    0, // EEVAR_FILAMENT_TYPE_3
    0, // EEVAR_FILAMENT_TYPE_4
    0, // EEVAR_FILAMENT_TYPE_5
    false, // EEVAR_HEATUP_BED
#if PRINTER_IS_PRUSA_XL()
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
    1, // EEVAR_HWCHECK_NOZZLE
    1, // EEVAR_HWCHECK_MODEL
    1, // EEVAR_HWCHECK_FIRMW
    1, // EEVAR_HWCHECK_GCODE
    {}, // EEVAR_SELFTEST_RESULT_PRE_23
};

#if PRINTER_IS_PRUSA_XL()
// first introduced footer items for XL
enum class FooterItems : uint8_t {
    Nozzle,
    Bed,
    Filament,
    FSensor,
    Speed,
    AxisX,
    AxisY,
    AxisZ,
    ZHeight,
    PrintFan,
    HeatbreakFan,
    Heatbreak,
    CurrentTool,
    AllNozzles,
    FSensorSide,
    None,
    _count
};

inline constexpr size_t old_footer_lines { 1 };
inline constexpr size_t old_footer_items_per_line { 5 };
using Record = std::array<FooterItems, old_footer_items_per_line>;
inline constexpr Record default_items = { { FooterItems::Nozzle,
    FooterItems::Bed,
    FooterItems::Filament,
    FooterItems::None,
    FooterItems::None } };

inline constexpr size_t count = old_footer_items_per_line;
inline constexpr size_t count_of_trailing_ones = 3;
inline constexpr size_t value_bit_size = 5; // 32 different items should be enough

/**
 * @brief encodes footer setting to uint32_t
 *
 * @param rec footer setting to encode
 * @return constexpr uint32_t
 */
constexpr uint32_t encode(Record rec) {
    uint32_t ret = uint32_t(rec[0]) << count_of_trailing_ones;
    for (size_t i = 1; i < count; ++i) {
        ret |= uint32_t(rec[i]) << ((value_bit_size * i) + count_of_trailing_ones);
    }
    // adding trailing ones to force default footer settings in FW version < 4.4.0 and using fixed size of footer item
    uint32_t trailing_ones = (1 << count_of_trailing_ones) - 1;
    ret |= trailing_ones;
    return ret;
}
#endif

// other printers are too young for this
union SelftestResultV1 {
    // values match TestResult - Unknown (0), Skipped (1), Passed (2), Failed(3)

    struct {
        uint8_t printFan : 2; // bit 0-1
        uint8_t heatBreakFan : 2; // bit 2-3
        uint8_t xaxis : 2; // bit 4-5
        uint8_t yaxis : 2; // bit 6-7
        uint8_t zaxis : 2; // bit 8-9
        uint8_t nozzle : 2; // bit 10-11
        uint8_t bed : 2; // bit 12-13
        uint8_t reserved0 : 2; // bit 14-15
        uint16_t reserved1; // bit 16-31
    };
    uint32_t ui32;
};

inline vars_body_t convert(const old_eeprom::v12::vars_body_t &src) {
    vars_body_t ret = body_defaults;

    // copy entire v12 struct
    memcpy(&ret, &src, sizeof(old_eeprom::v12::vars_body_t));
#if PRINTER_IS_PRUSA_XL()
    ret.FOOTER_SETTING = encode(default_items); // this is first XL version, so set default properly (even though this should never happen)
#endif

    SelftestResultV1 v1 { .ui32 = ret.SELFTEST_RESULT_V1 };

    ret.SELFTEST_RESULT_PRE_23.xaxis = static_cast<TestResult>(v1.xaxis);
    ret.SELFTEST_RESULT_PRE_23.yaxis = static_cast<TestResult>(v1.yaxis);
    ret.SELFTEST_RESULT_PRE_23.zaxis = static_cast<TestResult>(v1.zaxis);
    ret.SELFTEST_RESULT_PRE_23.bed = static_cast<TestResult>(v1.bed);
    ret.SELFTEST_RESULT_PRE_23.tools[0].printFan = static_cast<TestResult>(v1.printFan);
    ret.SELFTEST_RESULT_PRE_23.tools[0].heatBreakFan = static_cast<TestResult>(v1.heatBreakFan);
    ret.SELFTEST_RESULT_PRE_23.tools[0].nozzle = static_cast<TestResult>(v1.nozzle);

    return ret;
}

} // namespace config_store_ns::old_eeprom::v32787
