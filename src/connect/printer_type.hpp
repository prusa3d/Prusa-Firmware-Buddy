#pragma once

#include <cstdint>
#include <segmented_json_macros.h>
#include <printers.h>

inline constexpr uint8_t printer_type = PRINTER_TYPE;
inline constexpr uint8_t printer_version = PRINTER_VERSION;
inline constexpr uint8_t printer_subversion = PRINTER_SUBVERSION;

#define JSON_FIELD_PRINTER_TYPE JSON_FIELD_STR_FORMAT("printer_type", "%hhu.%hhu.%hhu", printer_type, printer_version, printer_subversion)
