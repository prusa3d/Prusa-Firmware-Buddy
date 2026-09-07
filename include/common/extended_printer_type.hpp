#pragma once

#include <array>
#include <atomic>
#include <cstdint>

#include <printers.h>
#include <common/printer_model.hpp>

// Some printer models share the same firmware, but have a slightly different hardware.
// ExtendedPrinterType (+ accompanying store item) serves to distinguish them.
//
// The stored index selects the effective PrinterModel, which is visible outside the printer
// (Connect printer type, USB PID, g-code checks, error-code prefixes). Consider it frozen to
// MK4/MK3.5/XL: new printer families should model removable equipment as config-store feature
// flags, optionally bundled into a PrinterVariant edition preset (see printer_variant.hpp for
// the decision rule); a printer gets at most one of the two mechanisms (enforced in
// printer_model.cpp).

#if PRINTER_IS_PRUSA_MK4()
    #define HAS_EXTENDED_PRINTER_TYPE()                    1
    #define EXTENDED_PRINTER_TYPE_DETERMINES_MOTOR_STEPS() 1
    #define IS_EXTENDED_PRINTER_TYPE_CONFIGURABLE()        1

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
    #define IS_EXTENDED_PRINTER_TYPE_CONFIGURABLE()        1

/// !!! Never change order, never remove items - this is used in config store
static constexpr std::array extended_printer_type_model {
    PrinterModel::mk3_5,
    PrinterModel::mk3_5s,
};

#elif PRINTER_IS_PRUSA_XL()
    #define HAS_EXTENDED_PRINTER_TYPE()                    1
    #define EXTENDED_PRINTER_TYPE_DETERMINES_MOTOR_STEPS() 0
    #define IS_EXTENDED_PRINTER_TYPE_CONFIGURABLE()        0 // Auto-detected based on XLCAN presence

/// !!! Never change order, never remove items - this is used in config store
static constexpr std::array extended_printer_type_model {
    PrinterModel::xl,
    PrinterModel::xls,
};

enum class XLTypeDetectionResult : uint8_t {
    ok,
    detected_as_xls,
    detected_as_xl,
    wiring_suspected,
};

/// Set by the puppy bootstrap, to be read by the marlin server during init
inline std::atomic<XLTypeDetectionResult> xl_type_detection_result = XLTypeDetectionResult::ok;

#else
    #define HAS_EXTENDED_PRINTER_TYPE()             0
    #define IS_EXTENDED_PRINTER_TYPE_CONFIGURABLE() 0

#endif

#if HAS_EXTENDED_PRINTER_TYPE()

enum class ChangeExtendedPrinterTypeMode {
    /// Accesses a marlin_client and puppies
    standard_with_marlin_client_and_puppies,

    /// Config-store changes only, assumes everything else is handled later
    config_store_init,
};

/// Apply a new extended printer type: write config store and update per-type side effects
void change_extended_printer_type(PrinterModel new_model, ChangeExtendedPrinterTypeMode mode);
#endif
