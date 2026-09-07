#pragma once
#include <fanctl/CFanCtlCommon.hpp>
#include "printers.h"
#include <device/board.h>
#include <cstddef>
#include <tool_index.hpp>
#include <option/has_indx.h>
#include <option/xl_enclosure_support.h>
#include <option/has_cpu_fan.h>

class Fans {
    Fans() = default;
    Fans(const Fans &) = default;

public:
    static CFanCtlCommon &print(PhysicalToolIndex tool);

    static CFanCtlCommon &heat_break(PhysicalToolIndex tool);

#if XL_ENCLOSURE_SUPPORT() // XLBOARD has CFanCtlPuppy and additional enclosure fan, but DWARF has only normal CFanCtls
    static CFanCtlCommon &enclosure();
#endif

#if HAS_INDX()
    // Auxiliary dock fan, wired to the xBuddy NEXTRUDER connector's print-fan
    // pin and using the same fan controller as the C1 print fan. The print fan
    // itself lives on the INDX head so that pin is free here.
    static CFanCtlCommon &dock_fan();
#endif

#if HAS_CPU_FAN()
    /// CPU cooling fan on the XLS sandwich board
    static CFanCtlCommon &cpu();
#endif
    static void tick();
    // Board-specific fan hardware init touching GPIO / I2C-expander pins; must run after hw_gpio_init().
    static void init_hw();
};
