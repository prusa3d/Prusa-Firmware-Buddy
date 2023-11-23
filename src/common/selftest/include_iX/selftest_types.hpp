/**
 * @file
 */

#pragma once

#ifdef __cplusplus
    #include <cstdint>
#else
    #include <stdint.h>
#endif

// TODO: Convert to enum class once marlin client is C++

enum ToolMask {
    NoneTools = 0, /**< NoneTools */
    AllTools = 0b11111111 /**< AllTools */
};
