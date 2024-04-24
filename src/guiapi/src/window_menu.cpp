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
    : AddSuperWindow<IWindowMenu>(parent, rect) {

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

void WindowMenu::windowEvent(EventLock /*has private ctor*/, [[maybe_unused]] window_t *sender, GUI_event_t event, void *param) {
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

    SuperWindowEvent(sender, event, param);
}

IWindowMenuItem *WindowMenu::itemFromSlot(int slot) {
    for (Node i = findFirst(); i.HasValue(); i = findNext(i)) {
        if (i.current_slot == slot) {
            return i.item; // found it
        }
    }
    return nullptr;
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

/**
 * @brief initializes menu from stored state
 * this will not work in case some items were hidden
 * @param var variant containing initialization data
 */
void WindowMenu::InitState(screen_init_variant::menu_t var) {
    move_focus_to_index(persistent_index_to_item_index(var.persistent_focused_index));
    set_scroll_offset(persistent_index_to_item_index(var.persistent_scroll_offset).value_or(0));
    ensure_item_on_screen(focused_item_index());
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

void WindowMenu::Show(IWindowMenuItem &item) {
    // Nothing to do
    if (!item.IsHidden()) {
        return;
    }

    if (!pContainer) {
        return;
    }

    pContainer->Show(item);

    if (!ensure_item_on_screen(focused_item_index())) {
        // screen did not roll, but some items still need invalidation
        std::optional<size_t> shown_index = GetIndex(item);
        if (!shown_index) {
            return; // this should never happen
        }

        // screen did not roll, but still must invalidate remaining items
        for (Node node = findFirst(); node.HasValue(); node = findNext(node)) {
            if (node.index > shown_index) {
                node.item->Invalidate();
            }
        }
    }
}

bool WindowMenu::Hide(IWindowMenuItem &item) {
    // Nothing to do
    if (item.IsHidden()) {
        return true;
    }

    const auto index_to_hide = GetIndex(item);

    // item is not member of container
    // normally could be hidden, but it is filtered by item.IsHidden() check
    if (!index_to_hide) {
        return false;
    }

    if (!pContainer->Hide(item)) {
        return false;
    }

    // screen might need to roll
    if (!ensure_item_on_screen(focused_item_index())) {
        // roll is not needed, but still must invalidate remaining items
        // hidden_index now points to first item after hidden one, so we need to invalidate it too
        for (Node node = findFirst(); node.HasValue(); node = findNext(node)) {
            if (node.index >= *index_to_hide) {
                node.item->Invalidate();
            }
        }
    }

    return true;
}

// unlike show / hide does not need any other action, number of items and positions remains the same
bool WindowMenu::SwapVisibility(IWindowMenuItem &item0, IWindowMenuItem &item1) {
    return pContainer && pContainer->SwapVisibility(item0, item1);
}

screen_init_variant::menu_t WindowMenu::GetCurrentState() const {
    return {
        .persistent_focused_index = static_cast<uint8_t>(item_index_to_persistent_index(focused_item_index()).value_or(-1)),
        .persistent_scroll_offset = static_cast<uint8_t>(item_index_to_persistent_index(scroll_offset()).value_or(0)),
    };
}
