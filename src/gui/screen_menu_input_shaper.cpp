#include "screen_menu_input_shaper.hpp"

#include <bit>

#include "RAII.hpp"

ScreenMenuInputShaper::ScreenMenuInputShaper()
    : detail::ScreenMenuInputShaper(_(label)) {

    update_gui();
}

void ScreenMenuInputShaper::update_gui() {
    if (is_updating_gui) {
        return;
    }

    // Also do not allow IS tuning mid-print
    const bool is_editing_enabled = !marlin_client::is_printing();

    AutoRestore _ar(is_updating_gui, true);

    const bool x_enabled = config_store().input_shaper_axis_x_enabled.get();
    const bool y_enabled = config_store().input_shaper_axis_y_enabled.get();

    Item<MI_IS_X_TYPE>().set_enabled(is_editing_enabled && x_enabled);
    Item<MI_IS_X_FREQUENCY>().set_enabled(is_editing_enabled && x_enabled);
    Item<MI_IS_Y_TYPE>().set_enabled(is_editing_enabled && y_enabled);
    Item<MI_IS_Y_FREQUENCY>().set_enabled(is_editing_enabled && y_enabled);
    Item<MI_IS_RESTORE_DEFAULTS>().set_enabled(is_editing_enabled);

    Item<MI_IS_X_TYPE>().set_show_disabled_extension(x_enabled);
    Item<MI_IS_X_FREQUENCY>().set_show_disabled_extension(x_enabled);
    Item<MI_IS_Y_TYPE>().set_show_disabled_extension(y_enabled);
    Item<MI_IS_Y_FREQUENCY>().set_show_disabled_extension(y_enabled);

    if (x_enabled) {
        const auto axis_config = config_store().input_shaper_axis_x_config.get();
        Item<MI_IS_X_TYPE>().update();
        Item<MI_IS_X_FREQUENCY>().SetVal(static_cast<int>(axis_config.frequency));
    }

    if (y_enabled) {
        const auto axis_config = config_store().input_shaper_axis_y_config.get();
        Item<MI_IS_Y_TYPE>().update();
        Item<MI_IS_Y_FREQUENCY>().SetVal(static_cast<int>(axis_config.frequency));
    }
}

void ScreenMenuInputShaper::windowEvent(window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::CHILD_CLICK) {
        switch (ftrstd::bit_cast<InputShaperMenuItemChildClickParam>(param)) {

        case InputShaperMenuItemChildClickParam::request_gui_update:
            update_gui();
            break;

        default:
            break;
        }

        return;
    }

    ScreenMenu::windowEvent(sender, event, param);
}
