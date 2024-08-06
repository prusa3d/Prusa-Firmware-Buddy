#pragma once

#include <array>

#include <printers.h>
#include <common/printer_model.hpp>

// Some printer models share the same firmware, but have a slightly different hardware.
// ExtendedPrinterType (+ accompanying store item) serves to distinguish them.

#if PRINTER_IS_PRUSA_MK4()
    #define HAS_EXTENDED_PRINTER_TYPE()                    1
    #define EXTENDED_PRINTER_TYPE_DETERMINES_MOTOR_STEPS() 1

/// !!! Never change order, never remove items - this is used in config store
static constexpr std::array extended_printer_type_model {
    PrinterModel::mk4,
    PrinterModel::mk4s,
    PrinterModel::mk3_9,
    PrinterModel::mk3_9s,
};

static constexpr std::array<bool, extended_printer_type_model.size()> extended_printer_type_has_400step_motors {
    true, // mk4
    true, // mk4s
    false, // mk3_9
    false, // mk3_9s
};

#elif PRINTER_IS_PRUSA_MK3_5()
    #define HAS_EXTENDED_PRINTER_TYPE()                    1
    #define EXTENDED_PRINTER_TYPE_DETERMINES_MOTOR_STEPS() 0

/// !!! Never change order, never remove items - this is used in config store
static constexpr std::array extended_printer_type_model {
    PrinterModel::mk3_5,
    PrinterModel::mk3_5s,
};

#else
    #define HAS_EXTENDED_PRINTER_TYPE() 0

#endif
