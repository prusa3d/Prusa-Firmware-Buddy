/// @file
#include "screen_selftest_submenu.hpp"

#include <tool_index.hpp>
#include <screen_menu_selftest_snake.hpp>
#include <selftest_action_helpers.hpp>
#include <bsod/bsod.h>

namespace SelftestSnake::screen_selftest_submenu {

Menu::Menu(window_t *parent, Rect16 rect)
    : WindowMenuVirtual(parent, rect, CloseScreenReturnBehavior::yes) {}

void Menu::setup(Action action) {
    action_ = action;
    setup_items();
}

int Menu::item_count() const {
    return PhysicalToolIndex::count + 1; // + MI_RETURN
}

// Returns a printf-style format string with a single %d for the 1-based tool/dock index.
constexpr auto get_submenu_label_template([[maybe_unused]] Action action) -> const char * {
    debug_assert(has_submenu(action));
#if PRINTER_IS_PRUSA_XL()
    switch (action) {
    case Action::DockCalibration:
        return N_("Dock %d Calibration");
    case Action::Loadcell:
        return N_("Tool %d Loadcell Test");
    case Action::FilamentSensorCalibration:
        return N_("Tool %d Filament Sensor Calibration");
    case Action::Gears:
        return N_("Tool %d Gearbox alignment");
    default:
        break;
    }
#elif HAS_INDX()
    if (action == Action::FilamentSensorCalibration) {
        return N_("Tool %d Filament Sensor Calibration");
    }
#endif
    return "Tool %d";
}

void Menu::setup_item(ItemVariant &variant, int index) {
    if (index == 0) {
        variant.emplace<MI_RETURN>();
    } else {
        const auto tool = PhysicalToolIndex::from_raw(index - 1);
        variant.emplace<I_MI_STS_SUBMENU>(get_submenu_label_template(action_), action_, tool);
    }
}

Screen::Screen(Action action)
    : ScreenMenuBase(nullptr, _(get_action_label(action)), EFooter::Off)
    , action_(action) {
    menu.menu.setup(action);
}

void Screen::draw() {
    if (is_menu_draw_enabled(this)) {
        window_frame_t::draw();
    }
}

void Screen::windowEvent(window_t *sender, GUI_event_t event, void *param) {
    do_menu_event(this, sender, event, param, action_, true);
}

} // namespace SelftestSnake::screen_selftest_submenu
