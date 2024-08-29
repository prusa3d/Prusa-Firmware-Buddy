#include "window_menu_virtual.hpp"

#include <ScreenHandler.hpp>
#include <sound.hpp>

void WindowMenuVirtualBase::setup_items() {
    const auto scroll_offset = this->scroll_offset();
    const auto item_count = this->item_count();

    for (int buffer_slot = 0; buffer_slot < item_buffer_size; buffer_slot++) {
        const auto index = buffer_slot_index(buffer_slot, scroll_offset);
        setup_buffer_slot(buffer_slot, index < item_count ? index : std::nullopt);
    }

    if (!items_set_up_ && should_focus_item_on_init()) {
        move_focus_to_index(0);
    }

    Invalidate();

    items_set_up_ = true;
}

IWindowMenuItem *WindowMenuVirtualBase::item_at(int index) {
    const auto scroll_offset = this->scroll_offset();
    if (index < scroll_offset || index >= scroll_offset + item_buffer_size) {
        return nullptr;
    }

    // We're intentionally not subtracting scroll_offset here
    // If we do modulo index, each index has a fixed position in the buffer,
    // so we don't have to shuffle the items around when scrolling
    return item_at_buffer_slot(index % item_buffer_size);
};

std::optional<int> WindowMenuVirtualBase::item_index(const IWindowMenuItem *item) const {
    if (!item) {
        return std::nullopt;
    }

    for (int buffer_slot = 0; buffer_slot < item_buffer_size; buffer_slot++) {
        if (const_cast<WindowMenuVirtualBase *>(this)->item_at_buffer_slot(buffer_slot) == item) {
            return buffer_slot_index(buffer_slot, scroll_offset());
        }
    }

    return std::nullopt;
}

void WindowMenuVirtualBase::set_scroll_offset(int set) {
    if (set == scroll_offset()) {
        return;
    }
    const auto previous_scroll_offset = scroll_offset();
    const int item_count = this->item_count();

    IWindowMenu::set_scroll_offset(set);

    for (int buffer_slot = 0; buffer_slot < item_buffer_size; buffer_slot++) {
        const auto old_index = buffer_slot_index(buffer_slot, previous_scroll_offset);
        const auto new_index = buffer_slot_index(buffer_slot, set);

        // If the buffer slot should now hold a different item, set it up
        if (new_index != old_index) {
            setup_buffer_slot(buffer_slot, (new_index < item_count) ? new_index : std::nullopt);
        }
    }
}

std::optional<int> WindowMenuVirtualBase::buffer_slot_index(int buffer_slot, int scroll_offset) const {
    // This is a reverse function to (buffer_slot = index % item_buffer_size) from item_at
    const auto result = scroll_offset + (buffer_slot + item_buffer_size - (scroll_offset % item_buffer_size)) % item_buffer_size;
    assert(result % item_buffer_size == buffer_slot);

    if (result >= scroll_offset + max_items_on_screen_count()) {
        return std::nullopt;
    }

    return result;
}

void WindowMenuVirtualBase::windowEvent(window_t *sender, GUI_event_t event, void *param) {
    switch (event) {

    case GUI_event_t::LOOP:
        for (int slot = 0; slot < item_buffer_size; slot++) {
            if (auto item = item_at_buffer_slot(slot)) {
                item->Loop();
            }
        }
        break;

    default:
        break;
    }

    IWindowMenu::windowEvent(sender, event, param);
}

void WindowMenuVirtualBase::screenEvent(window_t *sender, GUI_event_t event, void *param) {
    switch (event) {

    case GUI_event_t::TOUCH_SWIPE_LEFT:
    case GUI_event_t::TOUCH_SWIPE_RIGHT:
        if (bool(close_screen_return_behavior_)) {
            Sound_Play(eSOUND_TYPE::ButtonEcho);
            Screens::Access()->Close();
            return;
        }

    default:
        break;
    }

    IWindowMenu::screenEvent(sender, event, param);
}
