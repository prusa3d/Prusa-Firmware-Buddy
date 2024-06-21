#pragma once

#include <cstdint>
#include <inc/MarlinConfigPre.h>

enum class ToolMask : uint8_t {
    NoneTools = 0,
#if EXTRUDERS >= 2 // no tool options for single extruder
    Tool0 = 1,
    Tool1 = 1 << 1,
#endif
#if EXTRUDERS >= 3
    Tool2 = 1 << 2,
#endif
#if EXTRUDERS >= 4
    Tool3 = 1 << 3,
#endif
#if EXTRUDERS >= 5
    Tool4 = 1 << 4,
#endif
#if EXTRUDERS >= 6
    Tool5 = 1 << 5,
#endif
    AllTools = 0xff
};
