#pragma once

#include <cstdint>
#include <segmented_json_macros.h>
#include <printers.h>

inline constexpr uint8_t printer_type = PRINTER_TYPE;
inline constexpr uint8_t printer_version = PRINTER_VERSION;
inline constexpr uint8_t printer_subversion = PRINTER_SUBVERSION;

struct PrinterVersion {
    uint8_t type;
    uint8_t version;
    uint8_t subversion;
};

PrinterVersion get_printer_version();
