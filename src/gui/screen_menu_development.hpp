/// @file
#pragma once

#include "config_features.h" // HAS_TMC_WAVETABLE
#include "MItem_development.hpp"
#include "MItem_menus.hpp"
#include <basic_screen_menu.hpp>
#include <option/development_items.h>
#include <option/has_touch.h>
#include <option/has_attachable_accelerometer.h>

static_assert(DEVELOPMENT_ITEMS());

using ScreenMenuDevelopmentBase = BasicScreenMenu<
    MI_DRY_RUN,
#ifdef HAS_TMC_WAVETABLE
    MI_WAVETABLE_XYZ,
#endif
    MI_PRINTER_SETUP,
    MI_EXPERIMENTAL_SETTINGS,
    MI_ADVANCED_FOOTER,
    MI_ERROR_TEST,
#if HAS_TOUCH()
    MI_TOUCH_PLAYGROUND,
#endif
    MI_PRINTER_TYPE_CHANGED,
#if HAS_ATTACHABLE_ACCELEROMETER()
    MI_CHECK_ACCELEROMETER,
#endif
    MI_TRIGGER_BANK_MIGRATION>;

class ScreenMenuDevelopment final : public ScreenMenuDevelopmentBase {
public:
    ScreenMenuDevelopment();
};
