/**
 * @file screen_menu_diagnostics_mini.hpp
 * @brief Menu diagnostics for MINI printer
 */

#pragma once

#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include "MItem_tools.hpp"
#include "MItem_basic_selftest.hpp"

using ScreenMenuDiagnostics__ = ScreenMenu<GuiDefaults::MenuFooter, MI_RETURN, MI_SELFTEST, MI_MESH_BED, MI_TEST_FANS,
    MI_TEST_XYZ, MI_TEST_HEAT, MI_TEST_HOTEND, MI_TEST_BED, MI_SELFTEST_RESULT>;
