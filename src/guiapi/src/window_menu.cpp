// window_menu.cpp

#include <algorithm>
#include <cstdlib>
#include "window_menu.hpp"
#include "gui.hpp"
#include "sound.hpp"
#include "resource.h"
#include "WindowMenuItems.hpp"
#include "cmath_ext.h"

window_menu_t::window_menu_t(window_t *parent, Rect16 rect, IWinMenuContainer *pContainer, uint8_t index)
    : IWindowMenu(parent, rect)
    , moveIndex(0)
    , pContainer(pContainer) {
    setIndex(index);
    top_index = 0;
    updateTopIndex_IsRedrawNeeded();
}

//private, for ctor (cannot fail)
void window_menu_t::setIndex(uint8_t new_index) {
    if (new_index && (!pContainer))
        new_index = 0;
    if (new_index >= GetCount())
        new_index = 0;
    GetItem(new_index)->setFocus(); //set focus on new item
    index = new_index;
}

//public version of setIndex
bool window_menu_t::SetIndex(uint8_t index) {
    if (index && (!pContainer))
        return false; //cannot set non 0 without container
    if (index >= GetCount())
        return false;
    if (this->index == index)
        return true;
    IWindowMenuItem *activeItem = GetActiveItem();
    if (activeItem)
        activeItem->clrFocus(); //remove focus from old item
    GetItem(index)->setFocus(); //set focus on new item
    this->index = index;
    return true;
}

uint8_t window_menu_t::GetCount() const {
    if (!pContainer)
        return 0;
    return pContainer->GetCount();
}

IWindowMenuItem *window_menu_t::GetItem(uint8_t index) const {
    if (!pContainer)
        return nullptr;
    if (index >= GetCount())
        return nullptr;
    return pContainer->GetItem(index);
}

IWindowMenuItem *window_menu_t::GetActiveItem() {
    return GetItem(index);
}

bool window_menu_t::moveToNextVisibleItem() {
    if (moveIndex == 0)
        return true;
    int dir = SIGN1(moveIndex); /// direction of movement (+/- 1)

    IWindowMenuItem *item;
    int moved; // number of positions moved
    for (; moveIndex != 0; moveIndex -= dir) {
        moved = 0;
        do { /// skip all hidden items
            moved += dir;

            if (IS_OUT_OF_RANGE(index + moved, 0, GetCount() - 1)) {
                /// cursor would get out of menu
                moveIndex = 0;
                return false;
            }

            item = GetItem(index + moved);
            if (item == nullptr) {
                moveIndex = 0;
                return false;
            }
        } while (item->IsHidden());
        SetIndex(uint8_t(index + moved)); /// sets new cursor position to a visible item
    }
    return true;
}

int window_menu_t::visibleIndex(const int real_index) {
    int visible = -1; /// -1 => 0 items, 0 => 1 item, ...
    IWindowMenuItem *item;
    for (int i = 0; i < GetCount(); ++i) {
        item = GetItem(i);
        if (!item)
            return -1;
        if (!item->IsHidden())
            visible++;
        if (i == real_index)
            return std::max(0, visible);
    }
    return -1;
}

int window_menu_t::realIndex(const int visible_index) {
    int visible = -1; /// -1 => 0 items, 0 => 1 item, ...
    IWindowMenuItem *item;
    int i;
    for (i = 0; i < GetCount(); ++i) {
        item = GetItem(i);
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

bool window_menu_t::updateTopIndex_IsRedrawNeeded() {
    if (index < top_index) {
        top_index = index;
        return true; /// move the window up
    }

    if (index == top_index)
        return false;

    const int item_height = GuiDefaults::FontMenuItems->h + GuiDefaults::MenuPadding.top + GuiDefaults::MenuPadding.bottom;
    const int visible_available = Height() / item_height;

    const int visible_index = visibleIndex(index);

    if (visible_index < visibleIndex(top_index) + visible_available)
        return false; /// cursor is still in the window

    top_index = std::max(0, realIndex(visible_index - visible_available + 1));
    return true; /// move the window down
}

void window_menu_t::Increment(int dif) {
    moveIndex += dif; /// is not but could be atomic but should not hurt in GUI

    const int old_index = index;
    playEncoderSound(moveToNextVisibleItem()); /// moves index and plays a sound

    if (updateTopIndex_IsRedrawNeeded()) {
        // invalidate, but let invalid_background flag as it was
        // it will cause redraw of only invalid items
        bool back = flags.invalid_background;
        Invalidate();
        flags.invalid_background = back;
    } else {
        if (old_index != index) {
            /// just cursor moved, redraw affected items only
            GetItem(old_index)->Invalidate();
            GetItem(index)->Invalidate();
        }
    }
}

bool window_menu_t::playEncoderSound(bool changed) {
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
void window_menu_t::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
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
            item = GetItem(i);
            if (item)
                item->Loop();
        }
    } break;
    default:
        break;
    }
}

void window_menu_t::printItem(const size_t visible_count, IWindowMenuItem &item, const int item_height) {
    uint16_t rc_w = Width() - (GuiDefaults::MenuHasScrollbar ? GuiDefaults::MenuScrollbarWidth : 0);
    Rect16 rc = { Left(), int16_t(Top() + visible_count * (item_height + GuiDefaults::MenuItemDelimeterHeight)),
        rc_w, uint16_t(item_height) };

    if (GetRect().Contain(rc)) {

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
void window_menu_t::draw() {
    if (!IsVisible())
        return;

    bool setChildrenInvalid = IsInvalid(); // if background is invalid all items must be redrawn

    const int item_height = GuiDefaults::FontMenuItems->h + GuiDefaults::MenuPadding.top + GuiDefaults::MenuPadding.bottom;
    const size_t visible_available = Height() / item_height;
    size_t visible_count = 0;
    size_t available_invisible_count = 0;

    IWindowMenuItem *item = GetActiveItem();
    if (item) {
        for (size_t i = 0; i < GetCount(); ++i) {

            item = GetItem(i);
            if (!item)
                break;
            if (item->IsHidden())
                continue;
            if (visible_count < visible_available && i >= top_index) {

                if (setChildrenInvalid) {
                    item->Invalidate();
                }
                //this can draw just a port or entire item
                if (item->IsInvalid())
                    printItem(visible_count, *item, item_height);
            }
            if (i < top_index || visible_count >= visible_available) {
                available_invisible_count++;
            } else {
                visible_count++;
            }
        }
    }

    if constexpr (GuiDefaults::MenuHasScrollbar) {
        if (available_invisible_count) {
            printScrollBar(visible_count + available_invisible_count, visible_count);
        }
    }

    if (flags.invalid_background) {
        /// fill the rest of the window by background
        const int menu_h = visible_count * item_height;
        Rect16 rc_win = GetRect();
        rc_win -= Rect16::Height_t(menu_h);
        if (rc_win.Height() <= 0)
            return;
        rc_win += Rect16::Top_t(menu_h);
        display::FillRect(rc_win, GetBackColor());
    }
}

void window_menu_t::printScrollBar(size_t available_count, uint16_t visible_count) {
    uint16_t scroll_item_height = Height() / available_count;
    uint16_t sb_y_start = Top() + top_index * scroll_item_height;
    display::DrawRect(Rect16(int16_t(Left() + Width() - GuiDefaults::MenuScrollbarWidth), Top(), GuiDefaults::MenuScrollbarWidth, Height()), GetBackColor());
    display::DrawRect(Rect16(int16_t(Left() + Width() - GuiDefaults::MenuScrollbarWidth), sb_y_start, GuiDefaults::MenuScrollbarWidth, visible_count * scroll_item_height), COLOR_SILVER);
}

void window_menu_t::InitState(screen_init_variant::menu_t var) {
    SetIndex(var.index);
    top_index = var.top_index;
    updateTopIndex_IsRedrawNeeded();
}

void window_menu_t::SetContainer(IWinMenuContainer &cont, uint8_t index) {
    pContainer = &cont;
    setIndex(index); // ctor init fnc. version, valid to be used here
}

void window_menu_t::Show(IWindowMenuItem &item) {
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
        for (size_t i = pContainer->GetIndex(item) + 1; i < GetCount(); ++i) {
            IWindowMenuItem *pItem = GetItem(i);
            if (!pItem)
                return; // this should never happen

            pItem->Invalidate();
        }
    }
}

bool window_menu_t::Hide(IWindowMenuItem &item) {
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
        for (size_t i = pContainer->GetIndex(item) + 1; i < GetCount(); ++i) {
            IWindowMenuItem *pItem = GetItem(i);
            if (!pItem)
                return false; // this should never happen

            pItem->Invalidate();
        }
    }

    return true;
}

screen_init_variant::menu_t window_menu_t::GetCurrentState() const {
    return { index, top_index };
}
