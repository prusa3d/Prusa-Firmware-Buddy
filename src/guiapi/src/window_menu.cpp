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
    : AddSuperWindow<IWindowMenu>(parent, rect)
    , visible_count_at_last_draw(0) {

    if (pContainer) {
        BindContainer(*pContainer);
    }
}

bool WindowMenu::move_focus_to_index(std::optional<int> index) {
    if (!index) {
        IWindowMenuItem::move_focus(nullptr);
        return true;
    }

    if (pContainer && pContainer->SetIndex(*index)) {
        ensure_item_on_screen(index);
        return true;
    }

    return false;
}

std::optional<int> WindowMenu::focused_item_index() const {
    return pContainer ? pContainer->GetFocusedIndex() : std::nullopt;
}

int WindowMenu::item_count() const {
    return pContainer ? pContainer->GetVisibleCount() : 0;
}

IWindowMenuItem *WindowMenu::GetItem(int index) const {
    return pContainer ? pContainer->GetItemByVisibleIndex(index) : nullptr;
}

bool WindowMenu::playEncoderSound(bool changed) {
    if (changed) {
        Sound_Play(eSOUND_TYPE::EncoderMove); /// cursor moved normally
        return true;
    }
    Sound_Play(eSOUND_TYPE::BlindAlert); /// start or end of menu was hit by the cursor
    return false;
}

void WindowMenu::windowEvent(EventLock /*has private ctor*/, [[maybe_unused]] window_t *sender, GUI_event_t event, void *param) {
    const int value = int(param);

    IWindowMenuItem *focused_item = IWindowMenuItem::focused_item();

    // Check if the focused menu item is part of this menu
    if (focused_item && !GetIndex(*focused_item)) {
        focused_item = nullptr;
    }

    // Non-item-specific events
    switch (event) {

    case GUI_event_t::ENC_DN:
        if (focused_item && focused_item->is_edited()) {
            playEncoderSound(focused_item->Decrement(value));
            return;
        }
        break;

    case GUI_event_t::ENC_UP:
        if (focused_item && focused_item->is_edited()) {
            playEncoderSound(focused_item->Increment(value));
            return;
        }
        break;

    case GUI_event_t::CLICK:
        if (focused_item) {
            focused_item->Click(*this);
            return;
        }
        break;

    case GUI_event_t::CAPT_1:
        // TODO: change flag to checked
        break;

    case GUI_event_t::TEXT_ROLL:
        if (focused_item) {
            focused_item->Roll();
        }
        break;

    case GUI_event_t::TOUCH:
        if (auto focused_index = move_focus_touch_click(param); focused_index && pContainer) {
            const event_conversion_union event_data {
                .pvoid = param
            };
            pContainer->GetItemByVisibleIndex(*focused_index)->Touch(*this, event_data.point);
        }
        return;

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

void WindowMenu::set_scroll_offset(int set) {
    if (scroll_offset() == set) {
        return;
    }

    IWindowMenu::set_scroll_offset(set);

    // invalidate, but let invalid_background flag as it was
    // it will cause redraw of only invalid items
    bool back = flags.invalid_background;
    Invalidate();
    flags.invalid_background = back;
}

void WindowMenu::printItem(IWindowMenuItem &item, Rect16 rc) {
    // only place I know rectangle to be able to reinit roll, ugly to do it in print
    // TODO make some kind of roll event for menu items
    item.InitRollIfNeeded(rc);

    item.Print(rc);
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
void WindowMenu::draw() {
    if (!IsVisible()) {
        return;
    }

    bool setChildrenInvalid = IsInvalid(); // if background is invalid all items must be redrawn

    size_t drawn_cnt = 0;

    Node last_valid_node = Node::Empty();
    for (Node node = findFirst(); node.HasValue(); node = findNext(node)) {
        last_valid_node = node;
        if (setChildrenInvalid) {
            node.item->Invalidate();
        }
        // this can draw just a part or entire item
        if (node.item->IsInvalid()) {
            if (auto rect = slot_rect(node.current_slot); !rect.IsEmpty()) {
                printItem(*(node.item), rect);

                if constexpr (GuiDefaults::MenuLinesBetweenItems) {
                    if (flags.invalid_background && node.current_slot < current_items_on_screen_count() - 1) {
                        display::DrawLine(point_ui16(Left() + GuiDefaults::MenuItemDelimiterPadding.left, rect.Top() + rect.Height()),
                            point_ui16(Left() + Width() - GuiDefaults::MenuItemDelimiterPadding.right, rect.Top() + rect.Height()), COLOR_DARK_GRAY);
                    }
                }

                ++drawn_cnt;
            }
        }
    }

    // background is invalid or we used to have more items on screen
    // just redraw the rest of the window
    if (flags.invalid_background || visible_count_at_last_draw > drawn_cnt) {
        if (last_valid_node.HasValue()) {
            /// fill the rest of the window by background
            const int menu_h = (last_valid_node.current_slot + 1) * (item_height() + GuiDefaults::MenuItemDelimeterHeight);
            Rect16 rc_win = GetRect();
            rc_win -= Rect16::Height_t(menu_h);
            if (rc_win.Height() <= 0) {
                return;
            }
            rc_win += Rect16::Top_t(menu_h);
            display::FillRect(rc_win, GetBackColor());
        } else {
            // we dont have any items, just fill rectangle with back color
            unconditionalDraw();
        }
    }

    visible_count_at_last_draw = drawn_cnt;
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
    IWindowMenuItem *item = GetItem(scroll_offset());
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

    IWindowMenuItem *item = GetItem(prev.index + 1);
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
    move_focus_to_index(var.focused_index);
    set_scroll_offset(var.scroll_offset);
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
        .focused_index = static_cast<uint8_t>(focused_item_index().value_or(-1)),
        .scroll_offset = static_cast<uint8_t>(scroll_offset()),
    };
}
