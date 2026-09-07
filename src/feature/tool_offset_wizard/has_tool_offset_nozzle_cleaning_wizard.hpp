#pragma once

#include <option/has_indx.h>
#include <option/has_tool_offset_sensor.h>
#include <printers.h>

/// Whether the tool offset wizard includes the interactive nozzle cleaning
/// with heatup/cooldown dialog and released motors
#if !HAS_TOOL_OFFSET_SENSOR()
    #define HAS_TOOL_OFFSET_NOZZLE_CLEANING_WIZARD() 0
#elif HAS_INDX()
    #define HAS_TOOL_OFFSET_NOZZLE_CLEANING_WIZARD() 0
#elif PRINTER_IS_PRUSA_XL()
    #define HAS_TOOL_OFFSET_NOZZLE_CLEANING_WIZARD() 1
#else
    #error "Decide whether this printer gets the interactive nozzle cleaning"
#endif
