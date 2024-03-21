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
    is_editing_enabled = Item<MI_IS_ENABLE_EDITING>().IsHidden() && !marlin_client::is_printing();

    AutoRestore _ar(is_updating_gui, true);

    auto &is_config = input_shaper::current_config();

    const bool x_enabled = is_config.axis[X_AXIS].has_value();
    const bool y_enabled = is_config.axis[Y_AXIS].has_value();

    Item<MI_IS_X_ONOFF>().set_is_enabled(is_editing_enabled);
    Item<MI_IS_Y_ONOFF>().set_is_enabled(is_editing_enabled);
    Item<MI_IS_X_TYPE>().set_is_enabled(is_editing_enabled && x_enabled);
    Item<MI_IS_X_FREQUENCY>().set_is_enabled(is_editing_enabled && x_enabled);
    Item<MI_IS_Y_TYPE>().set_is_enabled(is_editing_enabled && y_enabled);
    Item<MI_IS_Y_FREQUENCY>().set_is_enabled(is_editing_enabled && y_enabled);
    Item<MI_IS_Y_COMPENSATION>().set_is_enabled(is_editing_enabled && y_enabled);
    Item<MI_IS_RESTORE_DEFAULTS>().set_is_enabled(is_editing_enabled);

    Item<MI_IS_X_TYPE>().set_show_disabled_extension(x_enabled);
    Item<MI_IS_X_FREQUENCY>().set_show_disabled_extension(x_enabled);
    Item<MI_IS_Y_TYPE>().set_show_disabled_extension(y_enabled);
    Item<MI_IS_Y_FREQUENCY>().set_show_disabled_extension(y_enabled);
    Item<MI_IS_Y_COMPENSATION>().set_show_disabled_extension(y_enabled);

    Item<MI_IS_X_ONOFF>().SetIndex(x_enabled);
    Item<MI_IS_Y_ONOFF>().SetIndex(y_enabled);
    Item<MI_IS_Y_COMPENSATION>().SetIndex(is_config.weight_adjust_y.has_value());

    if (x_enabled) {
        Item<MI_IS_X_TYPE>().SetIndex(static_cast<unsigned>(is_config.axis[X_AXIS]->type));
        Item<MI_IS_X_FREQUENCY>().SetVal(static_cast<int>(is_config.axis[X_AXIS]->frequency));
    }

    if (y_enabled) {
        Item<MI_IS_Y_TYPE>().SetIndex(static_cast<unsigned>(is_config.axis[Y_AXIS]->type));
        Item<MI_IS_Y_FREQUENCY>().SetVal(static_cast<int>(is_config.axis[Y_AXIS]->frequency));
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
