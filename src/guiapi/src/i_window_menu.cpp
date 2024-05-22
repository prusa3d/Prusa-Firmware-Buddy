#include "i_window_menu.hpp"

#include "marlin_client.hpp"
#include "sound.hpp"

#include <option/has_touch.h>
#if HAS_TOUCH()
    #include <hw/touchscreen/touchscreen.hpp>
#endif

IWindowMenu::IWindowMenu(window_t *parent, Rect16 rect)
    : AddSuperWindow<window_t>(parent, rect) {
    Enable();

    assert(height(GuiDefaults::FontMenuItems) == font_h_);
    max_items_on_screen_count_ = Height() / (item_height() + GuiDefaults::MenuItemDelimeterHeight);
}

void IWindowMenu::set_scroll_offset(int set) {
    assert(set >= 0 && set <= max_scroll_offset());
    scroll_offset_ = set;
}

bool IWindowMenu::ensure_item_on_screen(std::optional<int> opt_index) {
    if (!opt_index) {
        return false;
    }

    const int index = *opt_index;

    // The index is above viewport
    if (index < scroll_offset()) {
        set_scroll_offset(index);
        return true;
    }

    // The index is below the viewport
    else if (index >= scroll_offset() + max_items_on_screen_count()) {
        set_scroll_offset(std::min(index - max_items_on_screen_count() + 1, max_scroll_offset()));
        return true;
    }

    return false;
}

bool IWindowMenu::scroll_page(PageScrollDirection direction) {
    const int step_size = max_items_on_screen_count() - 1;

    const int new_scroll_offset = //
        (direction == PageScrollDirection::up) //
        ? std::max(scroll_offset() - step_size, 0)
        : std::min(scroll_offset() + step_size, max_scroll_offset());

    if (scroll_offset() == new_scroll_offset) {
        Sound_Play(eSOUND_TYPE::BlindAlert);
        return false;
    }

    // Play the sound before setting the index (set index could take some time, the sound response should be immediate)
    Sound_Play(eSOUND_TYPE::EncoderMove);

    set_scroll_offset(new_scroll_offset);
    marlin_client::notify_server_about_encoder_move();
    return true;
}

bool IWindowMenu::move_focus_by(int amount, YNPlaySound play_sound) {
    int new_index;

    if (auto opt_old_index = focused_item_index()) {
        const int old_index = *opt_old_index;
        new_index = (amount >= 0) ? std::min(old_index + amount, item_count() - 1) : std::max(old_index + amount, 0);

        if (new_index == old_index) {
            if (static_cast<bool>(play_sound)) {
                Sound_Play(eSOUND_TYPE::BlindAlert);
            }
            return false;
        }
    }

    // If nothing is focused, focus first element of the screen
    else {
        new_index = (amount >= 0) ? scroll_offset() : std::min(scroll_offset() + max_items_on_screen_count() - 1, item_count() - 1);
    }

    /// sets new cursor position to a visible item, also invalidates items at old and new index
    if (!move_focus_to_index(new_index)) {
        return false;
    }

    if (static_cast<bool>(play_sound)) {
        Sound_Play(eSOUND_TYPE::EncoderMove);
    }

    return true;
}

std::optional<int> IWindowMenu::move_focus_touch_click(void *event_param) {
    const event_conversion_union event_data {
        .pvoid = event_param
    };

    if (auto clicked_item_slot = slot_at_point(event_data.point)) {
        const auto target_index = *clicked_item_slot + scroll_offset();
        if (move_focus_to_index(target_index)) {
            return target_index;
        }
    }

    return std::nullopt;
}

bool IWindowMenu::should_focus_item_on_init() {
#if HAS_TOUCH()
    // If we're using touchscreen as a primary input, we don't want the first item in menu to be focused, because that's useful for encoder work, not touch
    if (touchscreen.is_enabled()) {
        return !GUI_event_is_touch_event(last_gui_input_event);
    }
#endif

    return true;
}

Rect16 IWindowMenu::slot_rect(int slot) const {
    if (slot < 0 || slot >= max_items_on_screen_count_) {
        return Rect16();
    }

    const Rect16 result {
        Left(),
        Rect16::Top_t(Top() + slot * (item_height() + GuiDefaults::MenuItemDelimeterHeight)),
        Width(),
        item_height(),
    };
    assert(GetRect().Contain(result));
    return result;
}

std::optional<int> IWindowMenu::slot_at_point(point_ui16_t point) const {
    for (int i = 0, end = current_items_on_screen_count(); i < end; ++i) {
        if (const auto rect = slot_rect(i); rect.Contain(point)) {
            return i;
        }
    }
    return std::nullopt;
}

std::optional<int> IWindowMenu::index_to_slot(std::optional<int> index) const {
    if (!index) {
        return std::nullopt;
    }

    const auto slot = *index - scroll_offset();
    if (slot < 0 || slot >= current_items_on_screen_count()) {
        return std::nullopt;
    }

    return slot;
}
void IWindowMenu::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    const int encoder_value = int(param);

    // Non-item-specific events
    switch (event) {

    case GUI_event_t::ENC_DN:
        move_focus_by(-encoder_value, YNPlaySound::yes);
        break;

    case GUI_event_t::ENC_UP:
        move_focus_by(encoder_value, YNPlaySound::yes);
        break;

    case GUI_event_t::TOUCH_CLICK:
        move_focus_touch_click(param);
        break;

    case GUI_event_t::TOUCH_SWIPE_DOWN:
        scroll_page(PageScrollDirection::up);
        break;

    case GUI_event_t::TOUCH_SWIPE_UP:
        scroll_page(PageScrollDirection::down);
        break;

    default:
        SuperWindowEvent(sender, event, param);
        break;
    }
}
