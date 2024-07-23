/**
 * @file screen_menu_version_info.hpp
 */

#pragma once

#include "printers.h"

// conditional parent alias include
#if PRINTER_IS_PRUSA_MINI()
    #include "screen_menu_version_info_mini.hpp"
#else
    #include "screen_menu_version_info_non_mini.hpp"
#endif // PRINTER_IS_PRUSA_MINI()
