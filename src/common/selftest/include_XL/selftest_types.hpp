#pragma once

#ifdef __cplusplus
    #include <cstdint>
#else
    #include <stdint.h>
#endif

// TODO: Convert to enum class once marlin client is C++

enum ToolMask {
    NoneTools = 0,
    ToolO = 1,
    Tool1 = 1 << 1,
    Tool2 = 1 << 2,
    Tool3 = 1 << 3,
    Tool4 = 1 << 4,
    Tool5 = 1 << 5,
    AllTools = 0b11111111
};
