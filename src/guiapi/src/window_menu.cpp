/**
 * @file window_menu.cpp
 */

#include <algorithm>
#include <cstdlib>
#include "window_menu.hpp"
#include "gui.hpp"
#include "sound.hpp"
#include "WindowMenuItems.hpp"
#include "cmath_ext.h"
#include "marlin_client.hpp"

WindowMenu::WindowMenu(window_t *parent, Rect16 rect, IWinMenuContainer *pContainer)
    : IWindowMenu(parent, rect) {

    if (pContainer) {
        BindContainer(*pContainer);
    }
}

std::optional<int> WindowMenu::item_index_to_persistent_index(std::optional<int> item_index) const {
    if (!item_index.has_value()) {
        return {};
    }

    IWindowMenuItem *item = pContainer->GetItemByVisibleIndex(*item_index);
    if (!item) {
        return {};
    }

    return pContainer->GetRawIndex(*item);
}

std::optional<int> WindowMenu::persistent_index_to_item_index(std::optional<int> persistent_index) const {
    if (!persistent_index.has_value()) {
        return {};
    }

    IWindowMenuItem *item = pContainer->GetItemByRawIndex(*persistent_index);
    if (!item) {
        return {};
    }

    return pContainer->GetVisibleIndex(*item);
}

int WindowMenu::item_count() const {
    return pContainer ? pContainer->GetVisibleCount() : 0;
}

IWindowMenuItem *WindowMenu::item_at(int index) {
    return pContainer ? pContainer->GetItemByVisibleIndex(index) : nullptr;
}

std::optional<int> WindowMenu::item_index(const IWindowMenuItem *item) const {
    return item && pContainer ? pContainer->GetVisibleIndex(*item) : std::nullopt;
}

void WindowMenu::windowEvent([[maybe_unused]] window_t *sender, GUI_event_t event, void *param) {
    // Non-item-specific events
    switch (event) {

    case GUI_event_t::TOUCH_SWIPE_LEFT:
    case GUI_event_t::TOUCH_SWIPE_RIGHT:
        for (auto it = pContainer->FindFirstVisible(); it.HasValue(); it = pContainer->FindNextVisible(it)) {
            if (it.item->has_return_behavior() && it.item->IsEnabled()) {
                Sound_Play(eSOUND_TYPE::ButtonEcho);

                // Move focus, because some returns items are handled based on focus by a parent class
                // cough cough screen_menu_filament_changeall::DMI_RETURN
                it.item->move_focus();

                // We don't need to repaint the item, really. Hopefully.
                // If we don't validate, we'll see a short flash of the item, no need
                it.item->Validate();

                it.item->Click(*this);
                return;
            }
        }
        break;

    case GUI_event_t::LOOP:
        for (Node i = findFirst(); i.HasValue(); i = findNext(i)) {
            i.item->Loop();
        }
        break;

    default:
        break;
    }

    IWindowMenu::windowEvent(sender, event, param);
}

WindowMenu::Node WindowMenu::findFirst() {
    IWindowMenuItem *item = item_at(scroll_offset());
    if (!item) {
        return Node::Empty();
    }
    Node ret = { item, 0, scroll_offset() };
    return ret;
}

WindowMenu::Node WindowMenu::findNext(WindowMenu::Node prev) {
    if (!prev.HasValue()) {
        return Node::Empty();
    }

    IWindowMenuItem *item = item_at(prev.index + 1);
    if (!item) {
        return Node::Empty();
    }
    Node ret = { item, prev.current_slot + 1, prev.index + 1 };
    return ret;
}

void WindowMenu::BindContainer(IWinMenuContainer &cont) {
    pContainer = &cont;

    set_scroll_offset(0);

    if (should_focus_item_on_init()) {
        move_focus_to_index(0);
    }
}

std::optional<int> WindowMenu::GetIndex(IWindowMenuItem &item) const {
    if (!pContainer) {
        return std::nullopt;
    }
    return pContainer->GetVisibleIndex(item);
}

// unlike show / hide does not need any other action, number of items and positions remains the same
bool WindowMenu::SwapVisibility(IWindowMenuItem &item0, IWindowMenuItem &item1) {
    return pContainer && pContainer->SwapVisibility(item0, item1);
}
