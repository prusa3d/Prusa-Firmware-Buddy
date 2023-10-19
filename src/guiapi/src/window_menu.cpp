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

WindowMenu::WindowMenu(window_t *parent, Rect16 rect, IWinMenuContainer *pContainer, uint8_t index)
    : AddSuperWindow<IWindowMenu>(parent, rect)
    , max_items_on_screen(Height() / (ItemHeight() + GuiDefaults::MenuItemDelimeterHeight))
    , visible_count_at_last_draw(0)
    , pContainer(pContainer) {
    setIndex(index);
    index_of_first = 0;
    updateTopIndex_IsRedrawNeeded(); // could use updateTopIndex() to invalidate affected items, but at this point everything is already invalid, so no need to do that
}

// private, for ctor (cannot fail)
void WindowMenu::setIndex(uint8_t new_index) {
    if (!pContainer)
        return;
    if (!pContainer->SetIndex(new_index))
        pContainer->SetIndex(0); // setting 0 could fail if menu has no visible item
                                 // but if it happens, container will have focused nullptr, which is valid
}

// public version of setIndex
bool WindowMenu::SetIndex(uint8_t index) {
    return pContainer && pContainer->SetIndex(index);
}

std::optional<size_t> WindowMenu::GetIndex() const {
    if (!pContainer)
        return std::nullopt;
    return pContainer->GetFocusedIndex();
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

IWindowMenuItem *WindowMenu::GetActiveItem() const {
    if (!pContainer)
        return nullptr;
    return pContainer->GetFocused();
}

bool WindowMenu::moveToNextVisibleItem(int moveIndex) {
    std::optional<size_t> opt_old_index = GetIndex();
    if (!opt_old_index)
        return false; // no item can be focused (container does not exist, or does not have visible items)

    int old_index = *opt_old_index;

    int new_index = (moveIndex >= 0) ? std::min(old_index + moveIndex, GetCount() - 1) : std::max(old_index + moveIndex, 0);
    if (new_index == old_index) // no move required
        return false; // TODO validate this false

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
    std::optional<size_t> opt_index = GetIndex();

    // no item can be focused (container does not exist, or does not have visible items), no redraw needed
    if (!opt_index)
        return false;
    uint8_t focused = *opt_index;

    // first item is focused, most common scenario, no redraw needed
    if (focused == index_of_first)
        return false;

    // some item before first is focused, roll the window up
    if (focused < index_of_first) {
        index_of_first = focused;
        return true;
    }

    // cursor is still in the window
    if (focused < (index_of_first + max_items_on_screen))
        return false;

    // cursor is behind visible area, roll the window down
    index_of_first = std::max(0, focused - max_items_on_screen + 1);
    return true;
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

void WindowMenu::windowEvent(EventLock /*has private ctor*/, [[maybe_unused]] window_t *sender, GUI_event_t event, void *param) {
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
        // TODO: change flag to checked
        break;
    case GUI_event_t::TOUCH: {
        if (!pContainer->GetFocusedIndex())
            return;
        event_conversion_union un;
        un.pvoid = param;
        std::optional<size_t> slot = slotFromCoord(un.point);
        if (!slot)
            break;

        // calculate diff
        int dif = int(*slot) - (int(*(pContainer->GetFocusedIndex())) - int(index_of_first));

        // set index
        if (dif != 0)
            Increment(dif);

        // do touched items action
        if (GetActiveItem()) {
            std::optional<Rect16> rc = getItemRC(*slot);
            if (rc) {
                point_ui16_t relative_touch_point = { un.point.x, un.point.y };
                GetActiveItem()->Touch(*this, relative_touch_point);
            }
        }
    } break;
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

/**
 * @brief roll method
 * uses other member function to calculate new index
 *
 * @param fn     member fnc pointer
 * @return true  changed
 * @return false unchanged
 */
bool WindowMenu::roll(roll_fn fn) {
    std::optional<uint8_t> new_index = std::invoke(fn, *this);

    if (!new_index)
        return false;

    if (pContainer && pContainer->SetIndex(*new_index)) {
        index_of_first = *new_index; // set focus to first
        Invalidate();
        return true;
    }

    return false;
}

/**
 * @brief calculates roll up index
 *
 * @return std::optional<uint8_t> new top and focused index
 */
std::optional<uint8_t> WindowMenu::calc_up_index() const {
    if (!pContainer)
        return std::nullopt;

    // roll impossible due small count if items
    if (pContainer->GetVisibleCount() <= max_items_on_screen) {
        return std::nullopt;
    }

    // we are at the top nothing to do
    if (index_of_first == 0) {
        return std::nullopt;
    }

    return uint8_t(std::max(int(index_of_first) - int(max_items_on_screen - 1), 0));
}

/**
 * @brief calculates roll down index
 *
 * @return std::optional<uint8_t> new top and focused index
 */
std::optional<uint8_t> WindowMenu::calc_down_index() const {
    if (!pContainer)
        return std::nullopt;

    uint8_t count = pContainer->GetVisibleCount();

    // roll impossible due small count if items
    if (count <= max_items_on_screen) {
        return std::nullopt;
    }

    // we are at the bottom nothing to do
    if (index_of_first >= (count - max_items_on_screen)) {
        return std::nullopt;
    }
    return std::min(index_of_first + max_items_on_screen - uint8_t(1), count - max_items_on_screen);
}

void WindowMenu::RollUp() {
    playEncoderSound(roll(&WindowMenu::calc_up_index));
}

void WindowMenu::RollDown() {
    playEncoderSound(roll(&WindowMenu::calc_down_index));
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
    // only place I know rectangle to be able to reinit roll, ugly to do it in print
    // TODO make some kind of roll event for menu items
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

    size_t drawn_cnt = 0;

    Node last_valid_node = Node::Empty();
    for (Node node = findFirst(); node.HasValue(); node = findNext(node)) {
        last_valid_node = node;
        if (setChildrenInvalid) {
            node.item->Invalidate();
        }
        // this can draw just a part or entire item
        if (node.item->IsInvalid()) {
            std::optional<Rect16> rc = getItemRC(node.current_slot);
            if (rc) {
                printItem(*(node.item), *rc);
                ++drawn_cnt;
            }
        }
    }

    // background is invalid or we used to have more items on screen
    // just redraw the rest of the window
    if (flags.invalid_background || visible_count_at_last_draw > drawn_cnt) {
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

    visible_count_at_last_draw = drawn_cnt;
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

    if (!pContainer)
        return;

    pContainer->Show(item);

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

    std::optional<size_t> index_to_hide = GetIndex(item);

    // item is not member of container
    // normally could be hidden, but it is filtered by item.IsHidden() check
    if (!index_to_hide)
        return false;

    if (!pContainer->Hide(item))
        return false;

    // screen might need to roll
    if (!updateTopIndex()) {
        // roll is not needed, but still must invalidate remaining items
        // hidden_index now points to first item after hidden one, so we need to invalidate it too
        for (Node node = findFirst(); node.HasValue(); node = findNext(node)) {
            if (node.index >= *index_to_hide)
                node.item->Invalidate();
        }
    }

    return true;
}

// unlike show / hide does not need any other action, number of items and positions remains the same
bool WindowMenu::SwapVisibility(IWindowMenuItem &item0, IWindowMenuItem &item1) {
    return pContainer && pContainer->SwapVisibility(item0, item1);
}

screen_init_variant::menu_t WindowMenu::GetCurrentState() const {
    return { GetIndex() ? uint8_t(*GetIndex()) : uint8_t(0), index_of_first };
}
