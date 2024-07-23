#pragma once

#include <enum_array.hpp>
#include <printers.h>

// Some printer models share the same firmware, but have a slightly different hardware.
// ExtendedPrinterType (+ accompanying store item) serves to distinguish them.

#if PRINTER_IS_PRUSA_MK4()
    #define HAS_EXTENDED_PRINTER_TYPE() 1

/// !!! Never change order, never remove items - this is used in config store
enum class ExtendedPrinterType : uint8_t {
    mk4 = 0, // Must be 0 - default value must be same for backwards compatibility with xy_motors_400_step (where it was true which meant mk4)
    mk4s = 1, // Set to this value on eeprom init in ConfigStore::perform_config_check
    mk3_9 = 2,
    _last = mk3_9,
};

static constexpr uint8_t extended_printer_type_count = ftrstd::to_underlying(ExtendedPrinterType::_last) + 1;

static constexpr EnumArray<ExtendedPrinterType, const char *, extended_printer_type_count> extended_printer_type_names {
    { ExtendedPrinterType::mk4, "MK4" },
    { ExtendedPrinterType::mk4s, "MK4S" },
    { ExtendedPrinterType::mk3_9, "MK3.9" },
};

static constexpr EnumArray<ExtendedPrinterType, bool, extended_printer_type_count> extended_printer_type_has_400step_motors {
    { ExtendedPrinterType::mk4, true },
    { ExtendedPrinterType::mk4s, true },
    { ExtendedPrinterType::mk3_9, false },
};

#else
    #define HAS_EXTENDED_PRINTER_TYPE() 0

#endif
