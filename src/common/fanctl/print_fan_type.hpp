#pragma once

#include <stdint.h>
#include <utils/enum_array.hpp>
#include <printers.h>
#include <i18n.h>
#include <option/has_print_fan_type.h>
#include <tool_index.hpp>

static_assert(HAS_PRINT_FAN_TYPE());

/// Maybe shared for other printers in future
/// !!! Never change order, never remove items - this is used in config store
enum class PrintFanType : uint8_t {
    DELTA_BFB0505HHA_CWCD = 0,
    GOM_VD_3706 = 1,
    LDO_D5015G08B05X71 = 2, // LDO blower for XLS
    _cnt,
};

#if PRINTER_IS_PRUSA_XL()
/// !!! MUST NOT CHANGE - CONFIG STORE DEFAULT
static constexpr PrintFanType default_print_fan_type = PrintFanType::DELTA_BFB0505HHA_CWCD;

static constexpr EnumArray<PrintFanType, const char *, PrintFanType::_cnt> print_fan_type_names {
    { PrintFanType::DELTA_BFB0505HHA_CWCD, N_("Black") },
    { PrintFanType::GOM_VD_3706, N_("Silver") },
    { PrintFanType::LDO_D5015G08B05X71, N_("LDO (XLS)") },
};

/// Filtered and ordered list of print fan types, for UI purposes
static constexpr std::array print_fan_type_list {
    PrintFanType::DELTA_BFB0505HHA_CWCD,
    PrintFanType::GOM_VD_3706,
    PrintFanType::LDO_D5015G08B05X71,
};
#else
    #error
#endif

PrintFanType get_print_fan_type(PhysicalToolIndex tool);
void set_print_fan_type(PhysicalToolIndex tool, PrintFanType pft);
uint16_t print_fan_remap_pwm(PrintFanType pft, uint16_t original_pwm);
