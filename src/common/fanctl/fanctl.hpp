#pragma once
#if defined(BOARD_IS_XLBUDDY) && BOARD_IS_XLBUDDY
    #include "CFanCtlPuppy.hpp"
    #include "CFanCtlEnclosure.hpp"
#else
    #include "CFanCtl.hpp"
#endif
#include "printers.h"
#include <device/board.h>
class Fans {
    Fans() = default;
    Fans(const Fans &) = default;

public:
#if defined(BOARD_IS_XLBUDDY) && BOARD_IS_XLBUDDY // XLBOARD has CFanCtlPuppy and additional enclosure fan, but DWARF has only normal CFanCtls
    static CFanCtlPuppy &print(size_t index);
    static CFanCtlPuppy &heat_break(size_t index);
    static CFanCtlEnclosure &enclosure();
#else
    static CFanCtl &print(size_t index);
    static CFanCtl &heat_break(size_t index);
#endif
    static void tick();
};
