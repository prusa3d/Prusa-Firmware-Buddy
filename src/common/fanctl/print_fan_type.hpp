#pragma once

#include <stdint.h>
#include <enum_array.hpp>
#include <printers.h>
#include <i18n.h>
#include <option/has_print_fan_type.h>

#if HAS_PRINT_FAN_TYPE()
/// Maybe shared for other printers in future
/// !!! Never change order, never remove items - this is used in config store
enum class PrintFanType : uint8_t {
    DELTA_BFB0505HHA_CWCD = 0,
    GOM_VD_2620 = 1,
    _cnt,
    #if PRINTER_IS_PRUSA_XL()
    default_value = DELTA_BFB0505HHA_CWCD,
    #endif // PRINTER_IS_PRUSA_XL()
};

static constexpr EnumArray<PrintFanType, const char *, PrintFanType::_cnt> print_fan_type_names {
    #if PRINTER_IS_PRUSA_XL()
    { PrintFanType::DELTA_BFB0505HHA_CWCD, N_("Black") },
        { PrintFanType::GOM_VD_2620, N_("Silver") },
    #endif // PRINTER_IS_PRUSA_XL()
};

/// Filtered and ordered list of print fan types, for UI purposes
static constexpr std::array print_fan_type_list {
    #if PRINTER_IS_PRUSA_XL()
    PrintFanType::DELTA_BFB0505HHA_CWCD,
        PrintFanType::GOM_VD_2620,
    #endif // PRINTER_IS_PRUSA_XL()
};

PrintFanType get_print_fan_type(size_t extruder_nr);
void set_print_fan_type(size_t extruder_nr, PrintFanType pft);
uint16_t print_fan_remap_pwm(PrintFanType pft, uint16_t original_pwm);

#endif // HAS_PRINT_FAN_TYPE()
