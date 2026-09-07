#pragma once

#include <utility>
#include <variant>
#include <optional>
#include <cinttypes>
#include <string_view>

using GCodeCheckPrinterModelCode = uint16_t;

/// Printer model enum is stored in EEPROM as last booted FW model. Order is important and should not change.
enum class PrinterModel : uint8_t {
    mk3 = 0,
    mk3s = 1,
    mk3_5 = 2,
    mk3_5s = 3,
    mk3_9 = 4,
    mk3_9s = 5,
    mk4 = 6,
    mk4s = 7,
    mini = 8,
    xl = 9,
    xl_dev_kit = 10,
    ix = 11,
    coreone = 12,
    coreonel = 13,
    coreone_indx = 14,
    coreonel_indx = 15,
    coreone_oak = 16,
    xls = 17,

    _cnt
};

enum class PrinterModelCompatibilityGroup : uint8_t {
    mk3,
    mk3_5,
    mk4,
    mk4s,
    xl,
    xls,
    ix,
    mini,
    coreone,
    coreonel,
    coreone_indx,
    coreonel_indx,
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
    bool is_compatible : 1 = false;

    /// Compatibility mode for running a MK3 gcode
    bool mk3_compatibility_mode : 1 = false;

    // Compatibility mode for running a MK4(non-S) gcode (or older)
    bool mk4_compatibility_mode : 1 = false;

    /// Compatibility mode for running an XL gcode on XLS (fan scaling)
    bool xl_compatibility_mode : 1 = false;

    /// Compatibility mode for running a non-chamber gcode
    bool chamber_compatibility_mode : 1 = false;

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

    /// String identifying the model for the user
    /// Defaults to id_str if not set
    const char *display_str_override = nullptr;

    inline uint16_t error_code_prefix() const {
        return usb_pid;
    }

    /// String identifying the model for the user
    inline const char *display_str() const {
        return display_str_override ?: id_str;
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
