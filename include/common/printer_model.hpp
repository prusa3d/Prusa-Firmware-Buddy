#pragma once

#include <utility>
#include <variant>
#include <optional>
#include <cinttypes>
#include <string_view>

using GCodeCheckPrinterModelCode = uint16_t;

/// Printer model enum. Should not be stored anywhere, the order of the items can change.
enum class PrinterModel : uint8_t {
    mk3,
    mk3s,
    mk3_5,
    mk3_5s,
    mk3_9,
    mk3_9s,
    mk4,
    mk4s,
    mini,
    xl,
    xl_dev_kit,
    ix,
    coreone,

    _cnt
};

enum class PrinterModelCompatibilityGroup : uint8_t {
    mk3,
    mk3_5,
    mk4,
    mk4s,
    xl,
    ix,
    mini,
    coreone,
};

struct PrinterVersion {

public:
    uint8_t type;
    uint8_t version;
    uint8_t subversion;

public:
    inline bool operator==(const PrinterVersion &) const = default;
    inline bool operator!=(const PrinterVersion &) const = default;
};

struct PrinterGCodeCompatibilityReport {

public:
    /// Whether the gcode can be run on the printer at all
    bool is_compatible = false;

    /// Whether the gcode should be run in MK3 compatibility mode
    bool mk3_compatibility_mode = false;

    /// Whether the fans should be run in the non-MK4S compatibility mode
    /// Meaning that this is a MK4S printer, GCode is sliced for a non-S printer, we need to lower the fan speeds due to a stronger fan
    bool mk4s_fan_compatibility_mode = false;

public:
    constexpr bool operator==(const PrinterGCodeCompatibilityReport &) const = default;
    constexpr bool operator!=(const PrinterGCodeCompatibilityReport &) const = default;
};

struct PrinterModelInfo {

public:
    PrinterModel model;

    PrinterModelCompatibilityGroup compatibility_group;

    /// Yet another way of encoding the printer model.
    /// This one is used for the Connect.
    PrinterVersion version;

    /// Identifier for help pages for the given printer
    const char *help_url;

    /// USB ID, also corresponds with error code prefix
    uint16_t usb_pid;

    /// Model code. Was used before in gcode checks.
    /// Not used anymore really, but we need to keep it for compatibility reasons.
    /// For MK3 and prior the values were assigned randomly.
    /// For MINI, MK4, ... and newer printers, first two numbers corespond to USB device ID and then are followed by zero.
    GCodeCheckPrinterModelCode gcode_check_code;

    /// String identifying the model for GCode checks
    const char *id_str;

    inline uint16_t error_code_prefix() const {
        return usb_pid;
    }

public:
    /// \returns model info of the specified printer model
    static const PrinterModelInfo &get(PrinterModel model);

    /// \returns model info of the specified printer model
    /// Requires including printer_model_data.hpp
    constexpr static const PrinterModelInfo &get_constexpr(PrinterModel model);

    /// \returns printer model info of the current printer
    static const PrinterModelInfo &current();

    /// \returns "base" printer model the current firmware is built for
    static const PrinterModelInfo &firmware_base();

    /// Looks up the printer model by the obsolete \p gcode_check_code. Also checks for the MMU variants.
    static const PrinterModelInfo *from_gcode_check_code(GCodeCheckPrinterModelCode code);

    /// Looks up the printer model by \p id_str. Also checks for the MMU variants.
    static const PrinterModelInfo *from_id_str(const std::string_view &id_str);

public:
    /// \returns compatibility report for when we're trying to print gcode sliced for \p gcode_target_printer on \p this printer
    PrinterGCodeCompatibilityReport gcode_compatibility_report(const PrinterModelInfo &gcode_target_printer) const;
};

/// Some printers have a MMU-variant records. This is actually not used anywhere and only kept for historical reasons. Maybe we should just get rid of it.
struct PrinterModelMMUVariant {
    PrinterModel model;

    /// The MMU version (two numbers) is prefixed to the printer number check code.
    uint16_t gcode_check_code;

    const char *id_str;
};
