/**
 * @file MItem_touch.cpp
 */
#include "MItem_touch.hpp"
#include "window_msgbox.hpp"
#include "img_resources.hpp"
#include "ScreenHandler.hpp"
#include <device/peripherals.h>
#include <hw/touchscreen/touchscreen.hpp>
#include "screen_touch_playground.hpp"

/*****************************************************************************/

MI_TOUCH_PLAYGROUND::MI_TOUCH_PLAYGROUND()
    : IWindowMenuItem {
        string_view_utf8::MakeCPUFLASH("Touch Playground"), // dev item, intentionally not translated
        nullptr,
        is_enabled_t::yes,
        is_hidden_t::dev,
    } {
}

void MI_TOUCH_PLAYGROUND::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenTouchPlayground>);
}

/*****************************************************************************/

MI_ENABLE_TOUCH::MI_ENABLE_TOUCH()
    : WI_ICON_SWITCH_OFF_ON_t(touchscreen.is_enabled(), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}

void MI_ENABLE_TOUCH::OnChange([[maybe_unused]] size_t old_index) {
    touchscreen.set_enabled(index);
}

/*****************************************************************************/

TOUCH_SIG_WORKAROUND::TOUCH_SIG_WORKAROUND()
    : WI_ICON_SWITCH_OFF_ON_t(config_store().touch_sig_workaround.get(), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}

void TOUCH_SIG_WORKAROUND::OnChange([[maybe_unused]] size_t old_index) {
    config_store().touch_sig_workaround.set(index);
}
