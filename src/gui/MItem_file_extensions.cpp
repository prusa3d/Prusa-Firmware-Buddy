/**
 * @file MItem_file_extensions.cpp
 */

#include "MItem_file_extensions.hpp"
#include <device/peripherals.h>
#include <hw/touchscreen/touchscreen.hpp>

MI_SHOW_FILE_EXTENSIONS::MI_SHOW_FILE_EXTENSIONS()
    : WI_ICON_SWITCH_OFF_ON_t(config_store().show_file_extensions.get(), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}

void MI_SHOW_FILE_EXTENSIONS::OnChange([[maybe_unused]] size_t old_index) {
    config_store().show_file_extensions.set(value());
}
