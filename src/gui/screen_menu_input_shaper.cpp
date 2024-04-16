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

    // If the "enable editing" button is hiden, enable editing by default
    // Also do not allow IS tuning mid-print
    // Editing disabled completely for 6.0.0 release - BFW-5297o
    is_editing_enabled = false && Item<MI_IS_ENABLE_EDITING>().IsHidden() && !marlin_client::is_printing();

    AutoRestore _ar(is_updating_gui, true);

    const bool x_enabled = config_store().input_shaper_axis_x_enabled.get();
    const bool y_enabled = config_store().input_shaper_axis_y_enabled.get();

    Item<MI_IS_X_ONOFF>().set_is_enabled(is_editing_enabled);
    Item<MI_IS_Y_ONOFF>().set_is_enabled(is_editing_enabled);
    Item<MI_IS_X_TYPE>().set_is_enabled(is_editing_enabled && x_enabled);
    Item<MI_IS_X_FREQUENCY>().set_is_enabled(is_editing_enabled && x_enabled);
    Item<MI_IS_Y_TYPE>().set_is_enabled(is_editing_enabled && y_enabled);
    Item<MI_IS_Y_FREQUENCY>().set_is_enabled(is_editing_enabled && y_enabled);
    Item<MI_IS_Y_COMPENSATION>().set_is_enabled(is_editing_enabled && y_enabled);
    Item<MI_IS_RESTORE_DEFAULTS>().set_is_enabled(!marlin_client::is_printing()); // TODO: revert back to is_editing_enabled

    Item<MI_IS_X_TYPE>().set_show_disabled_extension(x_enabled);
    Item<MI_IS_X_FREQUENCY>().set_show_disabled_extension(x_enabled);
    Item<MI_IS_Y_TYPE>().set_show_disabled_extension(y_enabled);
    Item<MI_IS_Y_FREQUENCY>().set_show_disabled_extension(y_enabled);
    Item<MI_IS_Y_COMPENSATION>().set_show_disabled_extension(y_enabled);

    Item<MI_IS_X_ONOFF>().SetIndex(x_enabled);
    Item<MI_IS_Y_ONOFF>().SetIndex(y_enabled);

    if (x_enabled) {
        const auto axis_config = config_store().input_shaper_axis_x_config.get();
        Item<MI_IS_X_TYPE>().SetIndex(static_cast<unsigned>(axis_config.type));
        Item<MI_IS_X_FREQUENCY>().SetVal(static_cast<int>(axis_config.frequency));
    }

    if (y_enabled) {
        const auto axis_config = config_store().input_shaper_axis_y_config.get();
        Item<MI_IS_Y_TYPE>().SetIndex(static_cast<unsigned>(axis_config.type));
        Item<MI_IS_Y_FREQUENCY>().SetVal(static_cast<int>(axis_config.frequency));
        Item<MI_IS_Y_COMPENSATION>().SetIndex(config_store().input_shaper_weight_adjust_y_enabled.get());
    }
}

void ScreenMenuInputShaper::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::CHILD_CLICK) {
        switch (ftrstd::bit_cast<InputShaperMenuItemChildClickParam>(param)) {

        case InputShaperMenuItemChildClickParam::request_gui_update:
            update_gui();
            break;

        case InputShaperMenuItemChildClickParam::enable_editing:
            is_editing_enabled = true;
            update_gui();
            break;

        default:
            break;
        }

        return;
    }

    SuperWindowEvent(sender, event, param);
}
