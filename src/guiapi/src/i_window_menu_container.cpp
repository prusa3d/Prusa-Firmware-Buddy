#include "i_window_menu_container.hpp"

IWinMenuContainer::Node IWinMenuContainer::FindFirstVisible() const {
    for (int i = 0; i < GetRawCount(); ++i) {
        IWindowMenuItem *item = GetItemByRawIndex(i);
        if (!item) {
            return Node::Empty();
        }
        if (item->IsHidden()) {
            continue;
        }
        Node ret = { item, i, 0 };
        return ret;
    }
    return Node::Empty();
}

IWinMenuContainer::Node IWinMenuContainer::FindNextVisible(Node prev) const {
    if (!prev.HasValue()) {
        return Node::Empty();
    }

    for (int i = prev.raw_index + 1; i < GetRawCount(); ++i) {
        IWindowMenuItem *item = GetItemByRawIndex(i);
        if (!item) {
            return Node::Empty();
        }
        if (item->IsHidden()) {
            continue;
        }
        Node ret = { item, i, prev.visible_index + 1 };
        return ret;
    }
    return Node::Empty();
}

IWindowMenuItem *IWinMenuContainer::GetItemByVisibleIndex(int pos) const {
    for (Node i = FindFirstVisible(); i.HasValue(); i = FindNextVisible(i)) {
        if (i.visible_index == pos) {
            return i.item; // found it
        }
    }
    return nullptr;
}

std::optional<int> IWinMenuContainer::GetVisibleIndex(IWindowMenuItem &item) const {
    for (Node i = FindFirstVisible(); i.HasValue(); i = FindNextVisible(i)) {
        if (i.item == &item) {
            return i.visible_index; // found it
        }
    }
    return std::nullopt;
}

int IWinMenuContainer::GetVisibleCount() const {
    int ret = 0;
    for (Node i = FindFirstVisible(); i.HasValue(); i = FindNextVisible(i)) {
        ret = i.visible_index + 1;
    }
    return ret;
}

IWindowMenuItem *IWinMenuContainer::GetVisibleItemWithOffset(IWindowMenuItem &item, int offset) const {
    std::optional<int> index = GetVisibleIndex(item);
    if (!index) {
        return nullptr;
    }
    int new_index = int(*index) + offset;
    if (new_index < 0) {
        return nullptr;
    }
    return GetItemByVisibleIndex(new_index);
}

bool IWinMenuContainer::SetIndex(int visible_index) {
    if (IWindowMenuItem *item = GetItemByVisibleIndex(visible_index)) {
        return item->move_focus();
    }

    return false;
}

std::optional<int> IWinMenuContainer::GetFocusedIndex() const {
    IWindowMenuItem *currently_focused = IWindowMenuItem::focused_item();

    if (!currently_focused) {
        return std::nullopt;
    }

    return GetVisibleIndex(*currently_focused);
}

bool IWinMenuContainer::Show(IWindowMenuItem &item) {
    if (GetRawIndex(item) == GetRawCount()) {
        return false; // not a member of container
    }

    if (GetVisibleIndex(item)) {
        return true; // already shown
    }

    item.show();

    // no need to modify currently_focused, since it is not index but pointer
    return true;
}

bool IWinMenuContainer::Hide(IWindowMenuItem &item) {
    if (GetRawIndex(item) == GetRawCount()) {
        return false; // not a member of container
    }

    if (!GetVisibleIndex(item)) {
        return true; // already hidden
    }

    if (item.is_focused()) {
        return false; // cannot hide focused item
    }

    item.hide();

    // no need to modify currently_focused, since it is not index but pointer
    return true;
}

bool IWinMenuContainer::SwapVisibility(IWindowMenuItem &item0, IWindowMenuItem &item1) {
    int32_t raw0 = GetRawIndex(item0);
    int32_t raw1 = GetRawIndex(item1);
    int32_t count = GetRawCount();

    if ((raw0 == count) || (raw1 == count)) {
        return false; // not a member of container
    }

    for (int32_t i = std::min(raw0, raw1) + 1; i < std::max(raw0, raw1); ++i) {
        if (GetItemByRawIndex(i) && !GetItemByRawIndex(i)->IsHidden()) {
            return false; // there must be no visible item between swapped items
        }
    }

    IWindowMenuItem *visible_item = nullptr;
    IWindowMenuItem *hidden_item = nullptr;

    (item0.IsHidden() ? hidden_item : visible_item) = &item0;
    (item1.IsHidden() ? hidden_item : visible_item) = &item1;

    if (!visible_item || !hidden_item) {
        return false; // both visible and hidden must be assigned
    }

    bool is_dev_only = visible_item->IsDevOnly();
    bool should_transfer_focus = visible_item->is_focused();

    // clear focus
    visible_item->clear_focus();

    // swap visibility
    visible_item->hide();
    is_dev_only ? hidden_item->showDevOnly() : hidden_item->show();

    // set focus to formal hidden
    if (should_transfer_focus) {
        hidden_item->move_focus();
    }

    return true;
}
