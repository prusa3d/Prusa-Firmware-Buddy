#include "MItem_redscreen.hpp"

MI_TRIGGER_REDSCREEN::MI_TRIGGER_REDSCREEN()
    : WI_LABEL_t(_("Trigger Redscreen"), nullptr, is_enabled_t::yes, is_hidden_t::dev, expands_t::yes) {
}

void MI_TRIGGER_REDSCREEN::click(IWindowMenu &) {
    fatal_error("Triggered from menu", "GUI");
}
