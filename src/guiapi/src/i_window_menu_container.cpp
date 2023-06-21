#include "i_window_menu_container.hpp"

IWinMenuContainer::Node IWinMenuContainer::FindFirstVisible() const {
    for (size_t i = 0; i < GetRawCount(); ++i) {
        IWindowMenuItem *item = GetItemByRawIndex(i);
        if (!item)
            return Node::Empty();
        if (item->IsHidden())
            continue;
        Node ret = { item, i, 0 };
        return ret;
    }
    return Node::Empty();
}

IWinMenuContainer::Node IWinMenuContainer::FindNextVisible(Node prev) const {
    if (!prev.HasValue())
        return Node::Empty();

    for (size_t i = prev.raw_index + 1; i < GetRawCount(); ++i) {
        IWindowMenuItem *item = GetItemByRawIndex(i);
        if (!item)
            return Node::Empty();
        if (item->IsHidden())
            continue;
        Node ret = { item, i, prev.visible_index + 1 };
        return ret;
    }
    return Node::Empty();
}

IWindowMenuItem *IWinMenuContainer::GetItemByVisibleIndex(size_t pos) const {
    for (Node i = FindFirstVisible(); i.HasValue(); i = FindNextVisible(i)) {
        if (i.visible_index == pos)
            return i.item; // found it
    }
    return nullptr;
}

std::optional<size_t> IWinMenuContainer::GetVisibleIndex(IWindowMenuItem &item) const {
    for (Node i = FindFirstVisible(); i.HasValue(); i = FindNextVisible(i)) {
        if (i.item == &item)
            return i.visible_index; // found it
    }
    return std::nullopt;
}

size_t IWinMenuContainer::GetVisibleCount() const {
    size_t ret = 0;
    for (Node i = FindFirstVisible(); i.HasValue(); i = FindNextVisible(i)) {
        ret = i.visible_index + 1;
    }
    return ret;
}

IWindowMenuItem *IWinMenuContainer::GetVisibleItemWithOffset(IWindowMenuItem &item, int offset) const {
    std::optional<size_t> index = GetVisibleIndex(item);
    if (!index)
        return nullptr;
    int new_index = int(*index) + offset;
    if (new_index < 0)
        return nullptr;
    return GetItemByVisibleIndex(new_index);
}

bool IWinMenuContainer::SetIndex(uint8_t visible_index) {
    if (visible_index >= GetVisibleCount())
        return false;

    IWindowMenuItem *to_be_focused = GetItemByVisibleIndex(visible_index);
    if (!to_be_focused)
        return false;

    if (currently_focused == to_be_focused) // OK, nothing to do
        return true;

    if (currently_focused)
        currently_focused->clrFocus(); // remove focus from old item
    to_be_focused->setFocus();         // set focus on new item

    // store currently focused index
    currently_focused = to_be_focused;
    return true;
}

std::optional<size_t> IWinMenuContainer::GetFocusedIndex() const {
    if (!currently_focused)
        return std::nullopt;

    return GetVisibleIndex(*currently_focused);
}

bool IWinMenuContainer::Show(IWindowMenuItem &item) {
    if (GetRawIndex(item) == GetRawCount())
        return false; // not a member of container

    if (GetVisibleIndex(item))
        return true; // already shown

    item.show();

    // no need to modify currently_focused, since it is not index but pointer
    return true;
}

bool IWinMenuContainer::Hide(IWindowMenuItem &item) {
    if (GetRawIndex(item) == GetRawCount())
        return false; // not a member of container

    if (!GetVisibleIndex(item))
        return true; // already hidden

    if (&item == currently_focused)
        return false; // cannot hide focused item

    item.hide();

    // no need to modify currently_focused, since it is not index but pointer
    return true;
}

bool IWinMenuContainer::SwapVisibility(IWindowMenuItem &item0, IWindowMenuItem &item1) {
    int32_t raw0 = GetRawIndex(item0);
    int32_t raw1 = GetRawIndex(item1);
    int32_t count = GetRawCount();

    if ((raw0 == count) || (raw1 == count))
        return false; // not a member of container

    for (int32_t i = std::min(raw0, raw1) + 1; i < std::max(raw0, raw1); ++i) {
        if (GetItemByRawIndex(i) && !GetItemByRawIndex(i)->IsHidden())
            return false; // there must be no visible item between swapped items
    }

    IWindowMenuItem *visible = nullptr;
    IWindowMenuItem *hidden = nullptr;

    item0.IsHidden() ? hidden = &item0 : visible = &item0;
    item1.IsHidden() ? hidden = &item1 : visible = &item1;

    if (!visible || !hidden) {
        return false; // both visible and hidden must be assigned
    }

    bool dev = visible->IsDevOnly();
    bool focus = visible == currently_focused;

    // clear focus
    visible->clrFocus();

    // swap visibility
    visible->hide();
    dev ? hidden->showDevOnly() : hidden->show();

    // set focus to formal hidden
    if (focus) {
        hidden->setFocus();
        currently_focused = hidden;
    }
    return true;
}
