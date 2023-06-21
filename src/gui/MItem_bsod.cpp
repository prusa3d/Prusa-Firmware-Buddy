#include "MItem_bsod.hpp"

MI_TRIGGER_BSOD::MI_TRIGGER_BSOD()
    : WI_LABEL_t(_("Trigger BSOD"), nullptr, is_enabled_t::yes, is_hidden_t::dev, expands_t::yes) {
}

void MI_TRIGGER_BSOD::click([[maybe_unused]] IWindowMenu &window_menu) {
    bsod("Triggered from menu");
}
