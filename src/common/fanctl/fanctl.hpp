#pragma once
#include "CFanCtlCommon.hpp"
#include "printers.h"
#include <device/board.h>
#include <cstddef>
class Fans {
    Fans() = default;
    Fans(const Fans &) = default;

public:
    static CFanCtlCommon &print(size_t index);
    static CFanCtlCommon &heat_break(size_t index);

#if XL_ENCLOSURE_SUPPORT() // XLBOARD has CFanCtlPuppy and additional enclosure fan, but DWARF has only normal CFanCtls
    static CFanCtlCommon &enclosure();
#endif
    static void tick();
};
