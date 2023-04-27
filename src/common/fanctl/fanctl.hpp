#include <device/board.h>
#include "Marlin/src/inc/MarlinConfig.h"
#include <array>

#if BOARD_IS_XLBUDDY
    #include "on_puppy/CFanCtlOnPuppy.hpp"

typedef CFanCtlOnPuppy CFanCtl;

#else
    #include "local/CFanCtlLocal.hpp"

typedef CFanCtlLocal CFanCtl;
#endif

extern std::array<CFanCtl, HOTENDS> fanCtlPrint;
extern std::array<CFanCtl, HOTENDS> fanCtlHeatBreak;

void fanctl_tick();
