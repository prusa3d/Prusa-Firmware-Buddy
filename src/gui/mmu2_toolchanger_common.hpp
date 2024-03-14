#pragma once

/**
 * @file mmu2_toolchanger_common.hpp
 * @brief It includes required headers and function declerations for different implementations (XL / MK4 with MMU2) of spool join
 */

#include <option/has_toolchanger.h>
#include "module/prusa/toolchanger.h"
#include <option/has_mmu2.h>
#if HAS_MMU2()
    #include <feature/prusa/MMU2/mmu2_mk4.h>
#endif

/**
 * XL and MK4 with MMU2 have different implementations. Definition of this function is in spool_join.cpp.
 */
bool is_tool_enabled(uint8_t idx);

/**
 * XL and MK4 with MMU2 have different implementations. Definition of this function is in screen_tools_mapping.cpp, where it is used.
 */
uint8_t get_num_of_enabled_tools();
