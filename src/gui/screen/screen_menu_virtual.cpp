/// @file
#include "screen_menu_virtual.hpp"
#include <bsod/bsod.h>

namespace screen_menu_virtual {

void Menu::set_configuration(const Configuration *configuration) {
    configuration_ = configuration;
    setup_items();
}

int Menu::item_count() const {
    return configuration_ ? configuration_->item_count() : 0;
}

void Menu::setup_item(ItemVariant &variant, int index) {
    debug_assert(configuration_);
    return configuration_->item_constructor(variant, index);
}

Screen::Screen(const Configuration *configuration)
    : ScreenMenuBase(nullptr, _(configuration->title), configuration->footer) {
    menu.menu.set_configuration(configuration);
}
} // namespace screen_menu_virtual
