#include "screen_menu_input_shaper.hpp"

ScreenMenuInputShaper::ScreenMenuInputShaper()
    : detail::ScreenMenuInputShaper(_(label)) {
}

void ScreenMenuInputShaper::windowEvent(EventLock /*has private ctor*/, [[maybe_unused]] window_t *sender, [[maybe_unused]] GUI_event_t event, [[maybe_unused]] void *param) {
    if (event == GUI_event_t::CHILD_CLICK) {
        input_shaper_param parameter = *(input_shaper_param *)param;
        switch (parameter) {
        case input_shaper_param::set_values:
            EnableItem<MI_IS_X_ONOFF>();
            EnableItem<MI_IS_Y_ONOFF>();
            if (Item<MI_IS_X_ONOFF>().GetIndex()) {
                EnableItem<MI_IS_X_TYPE>();
                EnableItem<MI_IS_X_FREQUENCY>();
            }
            if (Item<MI_IS_Y_ONOFF>().GetIndex()) {
                EnableItem<MI_IS_Y_TYPE>();
                EnableItem<MI_IS_Y_FREQUENCY>();
                EnableItem<MI_IS_Y_COMPENSATION>();
            }
            break;
        case input_shaper_param::change_x:
            if (Item<MI_IS_X_ONOFF>().GetIndex()) {
                EnableItem<MI_IS_X_TYPE>();
                EnableItem<MI_IS_X_FREQUENCY>();
            } else {
                Item<MI_IS_X_TYPE>().DontShowDisabledExtension();
                Item<MI_IS_X_FREQUENCY>().DontShowDisabledExtension();

                DisableItem<MI_IS_X_TYPE>();
                DisableItem<MI_IS_X_FREQUENCY>();
            }
            break;
        case input_shaper_param::change_y:
            if (Item<MI_IS_Y_ONOFF>().GetIndex()) {
                EnableItem<MI_IS_Y_TYPE>();
                EnableItem<MI_IS_Y_FREQUENCY>();
                EnableItem<MI_IS_Y_COMPENSATION>();
            } else {
                Item<MI_IS_Y_TYPE>().DontShowDisabledExtension();
                Item<MI_IS_Y_FREQUENCY>().DontShowDisabledExtension();
                Item<MI_IS_Y_COMPENSATION>().DontShowDisabledExtension();

                DisableItem<MI_IS_Y_TYPE>();
                DisableItem<MI_IS_Y_FREQUENCY>();
                DisableItem<MI_IS_Y_COMPENSATION>();
            }
            break;
        }
    }
}
