#include "i_window_menu.hpp"

#include "marlin_client.hpp"
#include <sound.hpp>
#include <i_window_menu_item.hpp>
#include "display.hpp"

#include <option/has_touch.h>
#if HAS_TOUCH()
    #include <hw/touchscreen/touchscreen.hpp>
#endif

IWindowMenu::IWindowMenu(window_t *parent, Rect16 rect)
    : window_t(parent, rect) {
    Enable();

    assert(height(GuiDefaults::FontMenuItems) == font_h_);
    max_items_on_screen_count_ = Height() / (item_height() + GuiDefaults::MenuItemDelimeterHeight);
}

void IWindowMenu::set_scroll_offset(int set) {
    assert(set >= 0 && set <= max_scroll_offset());
    scroll_offset_ = set;

    // invalidate, but let invalid_background flag as it was
    // it will cause redraw of only invalid items
    bool back = flags.invalid_background;
    Invalidate();
    flags.invalid_background = back;

    // The text roll needs to be invalidated, it has different coordinates now
    IWindowMenuItem::reset_roll();
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

std::optional<int> IWindowMenu::focused_item_index() const {
    return item_index(IWindowMenuItem::focused_item());
}

bool IWindowMenu::move_focus_to_index(std::optional<int> index) {
    if (!index) {
        IWindowMenuItem::move_focus(nullptr);
        return true;
    }

    ensure_item_on_screen(index);

    if (auto item = item_at(*index)) {
        item->move_focus();
        return true;
    }

    return false;
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

screen_init_variant::menu_t IWindowMenu::get_restore_state() const {
    return {
        .persistent_focused_index = item_index_to_persistent_index(focused_item_index()),
        .persistent_scroll_offset = item_index_to_persistent_index(scroll_offset()),
    };
}

void IWindowMenu::restore_state(screen_init_variant::menu_t state) {
    move_focus_to_index(persistent_index_to_item_index(state.persistent_focused_index));
    set_scroll_offset(persistent_index_to_item_index(state.persistent_scroll_offset).value_or(0));
    ensure_item_on_screen(focused_item_index());
}

/**
 * @brief menu behaves similar to frame
 * but redraw of background will not redraw area under items to avoid flickering
 *
 * flags.invalid            - all items are invalid
 * flags.invalid_background - background is invalid (lines between items too)
 *
 * does not use unconditionalDraw
 * unconditionalDraw would draw just black rectangle
 * which is same behavior as window_frame has
 */
void IWindowMenu::draw() {
    if (!IsVisible()) {
        return;
    }

    const bool redraw_all_items = IsInvalid();
    const int scroll_offset = this->scroll_offset();
    const int visible_slot_count = current_items_on_screen_count();

    for (int slot = 0; slot < visible_slot_count; slot++) {
        IWindowMenuItem *item = item_at(scroll_offset + slot);
        if (!item) {
            continue;
        }

        if (redraw_all_items) {
            item->Invalidate();
        }

        if (!item->IsInvalid()) {
            continue;
        }

        const auto rect = slot_rect(slot);
        if (rect.IsEmpty()) {
            continue;
        }

        item->Print(rect);

        if constexpr (GuiDefaults::MenuLinesBetweenItems) {
            if (flags.invalid_background && slot < visible_slot_count - 1) {
                display::draw_line(point_ui16(Left() + GuiDefaults::MenuItemDelimiterPadding.left, rect.Top() + rect.Height()),
                    point_ui16(Left() + Width() - GuiDefaults::MenuItemDelimiterPadding.right, rect.Top() + rect.Height()), COLOR_DARK_GRAY);
            }
        }
    }

    // background is invalid or we used to have more items on screen
    // just redraw the rest of the window
    if (flags.invalid_background || last_visible_slot_count_ > visible_slot_count) {
        if (visible_slot_count) {
            /// fill the rest of the window by background
            const int menu_h = visible_slot_count * (item_height() + GuiDefaults::MenuItemDelimeterHeight);
            Rect16 rc_win = GetRect();
            rc_win -= Rect16::Height_t(menu_h);
            if (rc_win.Height() <= 0) {
                return;
            }
            rc_win += Rect16::Top_t(menu_h);
            display::fill_rect(rc_win, GetBackColor());
        } else {
            // we dont have any items, just fill rectangle with back color
            unconditionalDraw();
        }
    }

    last_visible_slot_count_ = visible_slot_count;
}

void IWindowMenu::windowEvent(window_t *sender, GUI_event_t event, void *param) {
    const int encoder_value = int(param);

    IWindowMenuItem *focused_item = focused_item_index() ? IWindowMenuItem::focused_item() : nullptr;

    // Non-item-specific events
    switch (event) {

    case GUI_event_t::CLICK:
        if (focused_item) {
            focused_item->Click(*this);
        }
        break;

    case GUI_event_t::ENC_DN:
        if (focused_item && focused_item->is_edited()) {
            Sound_Play(focused_item->Decrement(std::bit_cast<int>(param)) ? eSOUND_TYPE::EncoderMove : eSOUND_TYPE::BlindAlert);
            break;
        }

        move_focus_by(-encoder_value, YNPlaySound::yes);
        break;

    case GUI_event_t::ENC_UP:
        if (focused_item && focused_item->is_edited()) {
            Sound_Play(focused_item->Increment(std::bit_cast<int>(param)) ? eSOUND_TYPE::EncoderMove : eSOUND_TYPE::BlindAlert);
            break;
        }

        move_focus_by(encoder_value, YNPlaySound::yes);
        break;

    case GUI_event_t::TOUCH_CLICK: {
        const auto focused_index = move_focus_touch_click(param);
        if (!focused_index) {
            break;
        }

        auto *item = item_at(*focused_index);
        if (!item) {
            break;
        }

        item->Touch(*this, event_conversion_union { param }.point);
        break;
    }

    case GUI_event_t::TOUCH_SWIPE_DOWN:
        scroll_page(PageScrollDirection::up);
        break;

    case GUI_event_t::TOUCH_SWIPE_UP:
        scroll_page(PageScrollDirection::down);
        break;

    case GUI_event_t::TEXT_ROLL:
        IWindowMenuItem::handle_roll();
        break;

    default:
        window_t::windowEvent(sender, event, param);
        break;
    }
}
