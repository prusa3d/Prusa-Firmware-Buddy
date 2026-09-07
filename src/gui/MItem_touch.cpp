/**
 * @file MItem_touch.cpp
 */
#include "MItem_touch.hpp"
#include <hw/touchscreen/touchscreen.hpp>
#include <config_store/store_instance.hpp>

/*****************************************************************************/

MI_ENABLE_TOUCH::MI_ENABLE_TOUCH()
    : WI_ICON_SWITCH_OFF_ON_t(touchscreen.is_enabled(), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}

void MI_ENABLE_TOUCH::OnChange([[maybe_unused]] size_t old_index) {
    touchscreen.set_enabled(value());
}

/*****************************************************************************/

TOUCH_SIG_WORKAROUND::TOUCH_SIG_WORKAROUND()
    : WI_ICON_SWITCH_OFF_ON_t(config_store().touch_sig_workaround.get(), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}

void TOUCH_SIG_WORKAROUND::OnChange([[maybe_unused]] size_t old_index) {
    config_store().touch_sig_workaround.set(value());
}
