// window_menu.cpp

#include <algorithm>
#include <cstdlib>
#include "window_menu.hpp"
#include "gui.hpp"
#include "sound.hpp"
#include "resource.h"
#include "IWindowMenuItem.hpp"
#include "cmath_ext.h"

IWindowMenu::IWindowMenu(window_t *parent, Rect16 rect)
    : window_aligned_t(parent, rect)
    , color_text(GuiDefaults::ColorText)
    , color_disabled(GuiDefaults::ColorDisabled)
    , font(GuiDefaults::Font)
    , padding { 6, 6, 6, 6 } {
    SetIconWidth(25);
    Enable();
}

uint8_t IWindowMenu::GetIconWidth() const {
    //mem_array_u08[0] is alignment
    return mem_array_u08[1];
}

void IWindowMenu::SetIconWidth(uint8_t width) {
    //mem_array_u08[0] is alignment
    mem_array_u08[1] = width;
    Invalidate();
}

window_menu_t::window_menu_t(window_t *parent, Rect16 rect, IWinMenuContainer *pContainer, uint8_t index)
    : IWindowMenu(parent, rect)
    , pContainer(pContainer) {
    /// Force to move cursor at the beginning to force complete redraw
    setIndex(index);
    moveIndex = 0;
}

//private, for ctor (cannot fail)
void window_menu_t::setIndex(uint8_t new_index) {
    if (new_index && (!pContainer))
        new_index = 0;
    if (new_index >= GetCount())
        new_index = 0;
    GetItem(new_index)->SetFocus(); //set focus on new item
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
        activeItem->ClrFocus(); //remove focus from old item
    GetItem(index)->SetFocus(); //set focus on new item
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
            if (index + dir >= GetCount() || index + dir < 0) {
                /// cursor would get out of menu
                moveIndex = 0;
                return false;
            }
            moved += dir;
            item = GetItem(index + moved);
            if (item == nullptr) {
                moveIndex = 0;
                return false;
            }
        } while (item->IsHidden());
        SetIndex(uint8_t(index + moved)); /// sets new cursor position (visible item)
    }
    return true;
}

int window_menu_t::visibleIndex(const int real_index) {
    int visible = 0;
    IWindowMenuItem *item;
    for (int i = 0; i < GetCount(); ++i) {
        item = GetItem(i);
        if (!item)
            return -1;
        if (!item->IsHidden())
            visible++;
        if (i == real_index)
            return visible;
    }
    return -1;
}

int window_menu_t::realIndex(const int visible_index) {
    int visible = 0;
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

bool window_menu_t::updateTopIndex() {
    if (index < top_index) {
        top_index = index;
        return true; /// move the window up
    }

    if (index == top_index)
        return false;

    const int item_height = font->h + padding.top + padding.bottom;
    const int visible_available = rect.Height() / item_height;

    const int visible_index = visibleIndex(index);

    if (visible_index < visibleIndex(top_index) + visible_available)
        return false; /// cursor is still in the window

    top_index = std::max(0, realIndex(visible_index - visible_available + 1));
    return true; /// move the window down
}

void window_menu_t::Increment(int dif) {
    moveIndex += dif; /// is not but could be atomic but should not hurt in GUI

    if (dif < 0) {
        moveIndex = -5;
    } else {
        moveIndex = 5;
    }

    Invalidate();
}

//I think I do not need
//screen_dispatch_event
//callback should handle it
void window_menu_t::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
    IWindowMenuItem *const item = GetActiveItem();
    if (!item)
        return;
    const int value = int(param);
    bool invalid = false;
    switch (event) {
    case GUI_event_t::CLICK:

        item->Click(*this);
        //Invalidate(); //called inside click
        break;
    case GUI_event_t::ENC_DN:
        if (item->IsSelected()) {
            invalid |= item->Decrement(value);
        } else {
            Decrement(value);
        }
        break;
    case GUI_event_t::ENC_UP:
        if (item->IsSelected()) {
            invalid |= item->Increment(value);
        } else {
            Increment(value);
        }
        break;
    case GUI_event_t::CAPT_1:
        //TODO: change flag to checked
        break;
    case GUI_event_t::TIMER:
        if (!item->RollNeedInit()) {
            item->Roll(*this); //warning it is accessing gui timer
        }
        break;
    default:
        break;
    }
    if (invalid)
        Invalidate();
}

void window_menu_t::printItem(const Rect16 &rect, const size_t visible_count, IWindowMenuItem *item, const int item_height) {
    if (item == nullptr)
        return;

    Rect16 rc = { rect.Left(), int16_t(rect.Top() + visible_count * item_height),
        rect.Width(), uint16_t(item_height) };

    if (rect.Contain(rc)) {
        if (item->RollNeedInit()) {
            gui_timer_restart_txtroll(this);
            gui_timer_change_txtroll_peri_delay(TEXT_ROLL_INITIAL_DELAY_MS, this);
            item->RollInit(*this, rc);
        }
        item->Print(*this, rc);
    }
}

void window_menu_t::unconditionalDraw() {
    if (moveIndex == 0) { /// forced redraw
        redrawWholeMenu();
        return;
    }

    IWindowMenuItem *item = GetActiveItem();
    if (!item) { /// weird state, fallback to first item
        index = 0;
        top_index = 0;
        moveIndex = 0;
        redrawWholeMenu();
        return;
    }

    if (item->IsSelected()) {
        if (item->Change(moveIndex)) {
            Sound_Play(eSOUND_TYPE::EncoderMove); // value changed
            unconditionalDrawItem(index);
        } else {
            Sound_Play(eSOUND_TYPE::BlindAlert); // value hitend of range
        }
        moveIndex = 0;
        return;
    }

    const int old_index = index;
    if (moveToNextVisibleItem()) {            /// changes index internally
        Sound_Play(eSOUND_TYPE::EncoderMove); // cursor moved normally
    } else {
        Sound_Play(eSOUND_TYPE::BlindAlert); // start or end of menu was hit by the cursor
        return;
    }

    if (updateTopIndex()) {
        redrawWholeMenu(); /// whole menu moved, redraw everything
    } else {
        unconditionalDrawItem(old_index); /// just cursor moved, redraw cursor only
        unconditionalDrawItem(index);
    }
}

void window_menu_t::redrawWholeMenu() {
    const int item_height = font->h + padding.top + padding.bottom;
    const size_t visible_available = rect.Height() / item_height;
    size_t visible_count = 0;
    IWindowMenuItem *item;
    for (size_t i = top_index; visible_count < visible_available && i < GetCount(); ++i) {

        item = GetItem(i);
        if (!item)
            break;
        if (item->IsHidden())
            continue;

        printItem(rect, visible_count, item, item_height);
        ++visible_count;
    }

    /// fill the rest of the window by background
    const int16_t menu_h = visible_count * item_height;
    Rect16 rc_win = rect;
    rc_win -= Rect16::Height_t(menu_h);
    if (rc_win.Height() <= 0)
        return;
    rc_win += Rect16::Top_t(menu_h);
    display::FillRect(rc_win, color_back);
}

void window_menu_t::unconditionalDrawItem(uint8_t index) {
    const int item_height = font->h + padding.top + padding.bottom;
    const size_t visible_available = rect.Height() / item_height;
    size_t visible_count = 0;
    IWindowMenuItem *item = nullptr;
    for (size_t i = top_index; visible_count < visible_available && i < GetCount(); ++i) {
        item = GetItem(i);
        if (!item)
            return;
        if (item->IsHidden())
            continue;
        if (i == index) {
            printItem(rect, visible_count, item, item_height);
            break;
        }
        ++visible_count;
    }
}
