/**
 * @file window_menu.cpp
 */

#include <algorithm>
#include <cstdlib>
#include "window_menu.hpp"
#include "gui.hpp"
#include "sound.hpp"
#include "resource.h"
#include "WindowMenuItems.hpp"
#include "cmath_ext.h"
#include "marlin_client.h"

WindowMenu::WindowMenu(window_t *parent, Rect16 rect, IWinMenuContainer *pContainer, uint8_t index)
    : AddSuperWindow<IWindowMenu>(parent, rect)
    , pContainer(pContainer) {
    setIndex(index);
    index_of_first = 0;
    updateTopIndex_IsRedrawNeeded();
}

//private, for ctor (cannot fail)
void WindowMenu::setIndex(uint8_t new_index) {
    if (new_index && (!pContainer))
        new_index = 0;
    if (new_index >= GetCount())
        new_index = 0;
    GetItemByRawIndex(new_index)->setFocus(); //set focus on new item
    index_of_focused = new_index;
}

//public version of setIndex
bool WindowMenu::SetIndex(uint8_t index) {
    if (index && (!pContainer))
        return false; //cannot set non 0 without container
    if (index >= GetCount())
        return false;
    if (this->index_of_focused == index)
        return true;
    IWindowMenuItem *activeItem = GetActiveItem();
    if (activeItem)
        activeItem->clrFocus();           //remove focus from old item
    GetItemByRawIndex(index)->setFocus(); //set focus on new item
    this->index_of_focused = index;
    return true;
}

uint8_t WindowMenu::GetCount() const {
    if (!pContainer)
        return 0;
    return pContainer->GetCount();
}

IWindowMenuItem *WindowMenu::GetItemByRawIndex(uint8_t index) const {
    if (!pContainer)
        return nullptr;
    if (index >= GetCount())
        return nullptr;
    return pContainer->GetItemByRawIndex(index);
}

IWindowMenuItem *WindowMenu::GetActiveItem() {
    return GetItemByRawIndex(index_of_focused);
}

// TODO
// this is over complicated
// container should be able to filter hidden items
bool WindowMenu::moveToNextVisibleItem(int moveIndex) {
    if (moveIndex == 0)
        return true;
    int dir = SIGN1(moveIndex); /// direction of movement (+/- 1)

    IWindowMenuItem *item;
    int moved; // number of positions moved
    for (; moveIndex != 0; moveIndex -= dir) {
        moved = 0;
        do { /// skip all hidden items
            moved += dir;

            if (IS_OUT_OF_RANGE(index_of_focused + moved, 0, GetCount() - 1)) {
                /// cursor would get out of menu
                moveIndex = 0;
                return false;
            }

            item = GetItemByRawIndex(index_of_focused + moved);
            if (item == nullptr) {
                moveIndex = 0;
                return false;
            }
        } while (item->IsHidden());
        SetIndex(uint8_t(index_of_focused + moved)); /// sets new cursor position to a visible item
    }
    return true;
}

int WindowMenu::visibleIndex(const int real_index) {
    int visible = -1; /// -1 => 0 items, 0 => 1 item, ...
    IWindowMenuItem *item;
    for (int i = 0; i < GetCount(); ++i) {
        item = GetItemByRawIndex(i);
        if (!item)
            return -1;
        if (!item->IsHidden())
            visible++;
        if (i == real_index)
            return std::max(0, visible);
    }
    return -1;
}

int WindowMenu::realIndex(const int visible_index) {
    int visible = -1; /// -1 => 0 items, 0 => 1 item, ...
    IWindowMenuItem *item;
    int i;
    for (i = 0; i < GetCount(); ++i) {
        item = GetItemByRawIndex(i);
        if (!item)
            return -1;
        if (!item->IsHidden())
            visible++;
        if (visible == visible_index)
            break;
    }

    if (visible == visible_index)
        return i;
    return -1;
}

bool WindowMenu::updateTopIndex_IsRedrawNeeded() {
    if (index_of_focused < index_of_first) {
        index_of_first = index_of_focused;
        return true; /// move the window up
    }

    if (index_of_focused == index_of_first)
        return false;

    const int item_height = GuiDefaults::FontMenuItems->h + GuiDefaults::MenuPaddingItems.top + GuiDefaults::MenuPaddingItems.bottom;
    const int visible_available = Height() / item_height;

    const int visible_index = visibleIndex(index_of_focused);

    if (visible_index < visibleIndex(index_of_first) + visible_available)
        return false; /// cursor is still in the window

    index_of_first = std::max(0, realIndex(visible_index - visible_available + 1));
    return true; /// move the window down
}

void WindowMenu::Increment(int dif) {
    const int old_index = index_of_focused;
    playEncoderSound(moveToNextVisibleItem(dif)); /// moves index and plays a sound

    if (updateTopIndex_IsRedrawNeeded()) {
        // invalidate, but let invalid_background flag as it was
        // it will cause redraw of only invalid items
        bool back = flags.invalid_background;
        Invalidate();
        flags.invalid_background = back;
    } else {
        if (old_index != index_of_focused) {
            /// just cursor moved, redraw affected items only
            GetItemByRawIndex(old_index)->Invalidate();
            GetItemByRawIndex(index_of_focused)->Invalidate();
        }
    }
}

bool WindowMenu::playEncoderSound(bool changed) {
    if (changed) {
        Sound_Play(eSOUND_TYPE::EncoderMove); /// cursor moved normally
        return true;
    }
    Sound_Play(eSOUND_TYPE::BlindAlert); /// start or end of menu was hit by the cursor
    return false;
}

//I think I do not need
//screen_dispatch_event
//callback should handle it
void WindowMenu::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    IWindowMenuItem *item = GetActiveItem();
    if (!item)
        return;
    const int value = int(param);
    switch (event) {
    case GUI_event_t::CLICK:
        item->Click(*this);
        break;
    case GUI_event_t::ENC_DN:
        if (item->IsSelected()) {
            playEncoderSound(item->Decrement(value));
        } else {
            Decrement(value);
        }
        break;
    case GUI_event_t::ENC_UP:
        if (item->IsSelected()) {
            playEncoderSound(item->Increment(value));
        } else {
            Increment(value);
        }
        break;
    case GUI_event_t::CAPT_1:
        //TODO: change flag to checked
        break;
    case GUI_event_t::TEXT_ROLL:
        item->Roll();
        break;
    case GUI_event_t::LOOP: {
        //it is simpler to send loop to all items, not just shown ones
        for (size_t i = 0; i < GetCount(); ++i) {
            item = GetItemByRawIndex(i);
            if (item)
                item->Loop();
        }
    } break;
    default:
        break;
    }
}

Rect16::Height_t WindowMenu::ItemHeight() {
    return GuiDefaults::FontMenuItems->h + GuiDefaults::MenuPadding.top + GuiDefaults::MenuPadding.bottom;
}

std::optional<Rect16> WindowMenu::getItemRC(size_t position_on_screen) const {
    Rect16 rc = { Left(), int16_t(Top() + position_on_screen * (ItemHeight() + GuiDefaults::MenuItemDelimeterHeight)), Width(), ItemHeight() };
    if (GetRect().Contain(rc))
        return rc;
    return std::nullopt;
}

void WindowMenu::printItem(IWindowMenuItem &item, Rect16 rc) {
    //only place I know rectangle to be able to reinit roll, ugly to do it in print
    //TODO make some kind of roll event for menu items
    item.InitRollIfNeeded(rc);

    item.Print(rc);

    // this should be elsewhere
    if constexpr (GuiDefaults::MenuLinesBetweenItems)
        if (flags.invalid_background)
            display::DrawLine(point_ui16(Left() + GuiDefaults::MenuItemDelimiterPadding.left, rc.Top() + rc.Height()),
                point_ui16(Left() + Width() - GuiDefaults::MenuItemDelimiterPadding.right, rc.Top() + rc.Height()), COLOR_DARK_GRAY);
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
    if (!IsVisible())
        return;

    bool setChildrenInvalid = IsInvalid(); // if background is invalid all items must be redrawn

    if (!GetActiveItem())
        return;

    Node last_valid_node = Node::Empty();
    for (Node node = findFirst(); node.HasValue(); node = findNext(node)) {
        if (!node.HasValue())
            break;
        if (node.item->IsHidden())
            continue;
        last_valid_node = node;
        if (setChildrenInvalid) {
            node.item->Invalidate();
        }
        //this can draw just a part or entire item
        if (node.item->IsInvalid()) {
            std::optional<Rect16> rc = getItemRC(node.current_slot);
            if (rc)
                printItem(*(node.item), *rc);
        }
    }

    if (flags.invalid_background) {
        if (last_valid_node.HasValue()) {
            /// fill the rest of the window by background
            const int menu_h = (last_valid_node.current_slot + 1) * (ItemHeight() + GuiDefaults::MenuItemDelimeterHeight);
            Rect16 rc_win = GetRect();
            rc_win -= Rect16::Height_t(menu_h);
            if (rc_win.Height() <= 0)
                return;
            rc_win += Rect16::Top_t(menu_h);
            display::FillRect(rc_win, GetBackColor());
        } else {
            // we dont have any items, just fill rectangle with back color
            unconditionalDraw();
        }
    }
}

std::optional<size_t> WindowMenu::slotFromCoord(point_ui16_t point) {
    const size_t visible_count = Height() / ItemHeight();
    for (size_t i = 0; i < visible_count; ++i) {
        if (!getItemRC(i))
            return std::nullopt;
        if (getItemRC(i)->Contain(point))
            return i;
    }
    return std::nullopt;
}

IWindowMenuItem *WindowMenu::itemFromSlot(size_t slot) {
    for (Node i = findFirst(); i.HasValue(); i = findNext(i)) {
        if (i.current_slot == slot)
            return i.item; // found it
    }
    return nullptr;
}

WindowMenu::Node WindowMenu::findFirst() {
    if (!GetActiveItem())
        return Node::Empty();
    const size_t slot_count = Height() / ItemHeight();

    size_t current_slot = 0;

    for (size_t i = index_of_first; i < GetCount(); ++i) {
        IWindowMenuItem *item = GetItemByRawIndex(i);
        if (!item)
            return Node::Empty();
        if (item->IsHidden())
            continue;
        Node ret = { item, slot_count, current_slot, i };
        return ret;
    }
    return Node::Empty();
}

WindowMenu::Node WindowMenu::findNext(WindowMenu::Node prev) {
    if (!prev.HasValue())
        return Node::Empty();

    if ((prev.current_slot + 1) >= prev.slot_count)
        return Node::Empty();

    for (size_t i = prev.real_index + 1; i < GetCount(); ++i) {
        IWindowMenuItem *item = GetItemByRawIndex(i);
        if (!item)
            return Node::Empty();
        if (item->IsHidden())
            continue;
        Node ret = { item, prev.slot_count, prev.current_slot + 1, i };
        return ret;
    }
    return Node::Empty();
}

void WindowMenu::printScrollBar(size_t available_count, uint16_t visible_count) {
    uint16_t scroll_item_height = Height() / available_count;
    uint16_t sb_y_start = Top() + index_of_first * scroll_item_height;
    display::DrawRect(Rect16(int16_t(Left() + Width() - GuiDefaults::MenuScrollbarWidth), Top(), GuiDefaults::MenuScrollbarWidth, Height()), GetBackColor());
    display::DrawRect(Rect16(int16_t(Left() + Width() - GuiDefaults::MenuScrollbarWidth), sb_y_start, GuiDefaults::MenuScrollbarWidth, visible_count * scroll_item_height), COLOR_SILVER);
}

void WindowMenu::InitState(screen_init_variant::menu_t var) {
    SetIndex(var.index);
    index_of_first = var.top_index;
    updateTopIndex_IsRedrawNeeded();
}

void WindowMenu::BindContainer(IWinMenuContainer &cont, uint8_t index) {
    pContainer = &cont;
    setIndex(index); // ctor init fnc. version, valid to be used here
}

void WindowMenu::Show(IWindowMenuItem &item) {
    if (!pContainer)
        return;

    // Nothing to do
    if (!item.IsHidden()) {
        return;
    }

    item.show();

    // screen might need to roll
    if (updateTopIndex_IsRedrawNeeded()) {
        // invalidate, but let invalid_background flag as it was
        // it will cause redraw of only invalid items
        bool back = flags.invalid_background;
        Invalidate();
        flags.invalid_background = back;
    } else {
        // roll is not needed, but still must invalidate remaining items
        for (size_t i = pContainer->GetRawIndex(item) + 1; i < GetCount(); ++i) {
            IWindowMenuItem *pItem = GetItemByRawIndex(i);
            if (!pItem)
                return; // this should never happen

            pItem->Invalidate();
        }
    }
}

bool WindowMenu::Hide(IWindowMenuItem &item) {
    if (!pContainer)
        return false;

    // Nothing to do
    if (item.IsHidden()) {
        return true;
    }

    item.hide();

    flags.invalid_background = true; // might need to draw empty rect over last item

    // screen might need to roll
    if (updateTopIndex_IsRedrawNeeded()) {
        Invalidate();
    } else {
        // roll is not needed, but still must invalidate remaining items
        for (size_t i = pContainer->GetRawIndex(item) + 1; i < GetCount(); ++i) {
            IWindowMenuItem *pItem = GetItemByRawIndex(i);
            if (!pItem)
                return false; // this should never happen

            pItem->Invalidate();
        }
    }

    return true;
}

screen_init_variant::menu_t WindowMenu::GetCurrentState() const {
    return { index_of_focused, index_of_first };
}
