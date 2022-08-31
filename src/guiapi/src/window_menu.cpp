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
#include "marlin_client.hpp"

WindowMenu::WindowMenu(window_t *parent, Rect16 rect, IWinMenuContainer *pContainer, uint8_t index)
    : AddSuperWindow<IWindowMenu>(parent, rect)
    , max_items_on_screen(Height() / (ItemHeight() + GuiDefaults::MenuItemDelimeterHeight))
    , pContainer(pContainer) {
    setIndex(index);
    index_of_first = 0;
    updateTopIndex_IsRedrawNeeded(); // could use updateTopIndex() to invalidate affected items, but at this point everything is already invalid, so no need to do that
}

//private, for ctor (cannot fail)
void WindowMenu::setIndex(uint8_t new_index) {
    if (new_index && (!pContainer))
        new_index = 0;
    if (new_index >= GetCount())
        new_index = 0;
    auto item = GetItem(new_index);
    if (item)
        item->setFocus(); //set focus on new item
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
        activeItem->clrFocus(); //remove focus from old item
    GetItem(index)->setFocus(); //set focus on new item
    this->index_of_focused = index;
    return true;
}

uint8_t WindowMenu::GetCount() const {
    if (!pContainer)
        return 0;
    return pContainer->GetVisibleCount();
}

IWindowMenuItem *WindowMenu::GetItem(uint8_t index) const {
    if (!pContainer)
        return nullptr;
    return pContainer->GetItemByVisibleIndex(index);
}

IWindowMenuItem *WindowMenu::GetActiveItem() {
    return GetItem(index_of_focused);
}

bool WindowMenu::moveToNextVisibleItem(int moveIndex) {
    int old_index = index_of_focused;
    // in case container does not exist index_of_focused == 0 and new_index will be 0, so this state is OK
    int new_index = (moveIndex >= 0) ? std::min(old_index + moveIndex, GetCount() - 1) : std::max(old_index + moveIndex, 0);
    if (new_index == int(old_index)) // cannot move
        return false;

    /// sets new cursor position to a visible item, also invalidates items at old and new index
    if (!SetIndex(new_index))
        return false;

    // redraw affected items between ones which lost and gained focus (already invalid)
    for (int index = std::min(old_index, new_index) + 1; index < std::max(old_index, new_index); ++index) {
        IWindowMenuItem *item = GetItem(index);
        if (item)
            item->Invalidate();
    }
    return true;
}

bool WindowMenu::updateTopIndex_IsRedrawNeeded() {
    if (index_of_focused < index_of_first) {
        index_of_first = index_of_focused;
        return true; /// move the window up
    }

    if (index_of_focused == index_of_first)
        return false;

    if (index_of_focused < (index_of_first + max_items_on_screen))
        return false; /// cursor is still in the window

    index_of_first = std::max(0, index_of_focused - max_items_on_screen + 1);
    return true; /// move the window down
}

bool WindowMenu::updateTopIndex() {
    if (updateTopIndex_IsRedrawNeeded()) {
        // invalidate, but let invalid_background flag as it was
        // it will cause redraw of only invalid items
        bool back = flags.invalid_background;
        Invalidate();
        flags.invalid_background = back;
        return true; /// move the window up
    }
    return false;
}

void WindowMenu::Increment(int dif) {
    playEncoderSound(moveToNextVisibleItem(dif)); /// moves index and plays a sound

    updateTopIndex();
}

bool WindowMenu::playEncoderSound(bool changed) {
    if (changed) {
        Sound_Play(eSOUND_TYPE::EncoderMove); /// cursor moved normally
        return true;
    }
    Sound_Play(eSOUND_TYPE::BlindAlert); /// start or end of menu was hit by the cursor
    return false;
}

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
        for (Node i = findFirst(); i.HasValue(); i = findNext(i)) {
            i.item->Loop();
        }
    } break;
    default:
        break;
    }
}

Rect16::Height_t WindowMenu::ItemHeight() {
    return GuiDefaults::FontMenuItems->h + GuiDefaults::MenuPaddingItems.top + GuiDefaults::MenuPaddingItems.bottom;
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
    IWindowMenuItem *item = GetItem(index_of_first);
    if (!item)
        return Node::Empty();
    Node ret = { item, 0, index_of_first };
    return ret;
}

WindowMenu::Node WindowMenu::findNext(WindowMenu::Node prev) {
    if (!prev.HasValue())
        return Node::Empty();

    IWindowMenuItem *item = GetItem(prev.index + 1);
    if (!item)
        return Node::Empty();
    Node ret = { item, prev.current_slot + 1, prev.index + 1 };
    return ret;
}

/**
 * @brief initializes menu from stored state
 * this will not work in case some items were hidden
 * @param var variant containing initialization data
 */
void WindowMenu::InitState(screen_init_variant::menu_t var) {
    SetIndex(var.index);
    index_of_first = var.top_index;
    updateTopIndex();
}

void WindowMenu::BindContainer(IWinMenuContainer &cont, uint8_t index) {
    pContainer = &cont;
    setIndex(index); // ctor init fnc. version, valid to be used here
}

std::optional<size_t> WindowMenu::GetIndex(IWindowMenuItem &item) const {
    if (!pContainer)
        return std::nullopt;
    return pContainer->GetVisibleIndex(item);
}

void WindowMenu::Show(IWindowMenuItem &item) {
    // Nothing to do
    if (!item.IsHidden()) {
        return;
    }

    item.show();

    if (!updateTopIndex()) {
        // screen did not roll, but some items still need invalidation
        std::optional<size_t> shown_index = GetIndex(item);
        if (!shown_index)
            return; // this should never happen

        // screen did not roll, but still must invalidate remaining items
        for (Node node = findFirst(); node.HasValue(); node = findNext(node)) {
            if (node.index > shown_index)
                node.item->Invalidate();
        }
    }
}

bool WindowMenu::Hide(IWindowMenuItem &item) {
    // Nothing to do
    if (item.IsHidden()) {
        return true;
    }

    std::optional<size_t> hidden_index = GetIndex(item);
    if (!hidden_index)
        return false; // this should never happen
    item.hide();

    // screen might need to roll
    if (!updateTopIndex()) {
        // roll is not needed, but still must invalidate remaining items
        // hidden_index now points to first item after hidden one, so we need to invalidate it too
        for (Node node = findFirst(); node.HasValue(); node = findNext(node)) {
            if (node.index >= hidden_index)
                node.item->Invalidate();
        }
    }

    return true;
}

screen_init_variant::menu_t WindowMenu::GetCurrentState() const {
    return { index_of_focused, index_of_first };
}
