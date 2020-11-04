/**
 * @file menu_spin_config.hpp
 * @author Radek Vana
 * @brief
 * @version 0.1
 * @date 2020-11-04
 *
 * @copyright Copyright (c) 2020
 *
 */
#pragma once

#if (PRINTER_TYPE == PRINTER_PRUSA_MINI)
    #include "menu_spin_config_basic.hpp"
using SpinCnf = DoNotUse::SpinCnf_basic; // DoNotUse is meant to be used here
#else
    #include "menu_spin_config_with_units.hpp"
using SpinCnf = DoNotUse::SpinCnf_with_units; // DoNotUse is meant to be used here
#endif
