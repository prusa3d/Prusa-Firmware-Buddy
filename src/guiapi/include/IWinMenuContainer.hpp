#pragma once

#include <stdint.h>
#include <optional>
#include "WindowMenuItems.hpp"

//todo
//I want same methods for IWinMenuContainer as std::array<IWindowMenuItem *, N> .. need to add iterators
class IWinMenuContainer {
public:
    virtual size_t GetCount() = 0;
    virtual IWindowMenuItem *GetItemByRawIndex(size_t pos) = 0;
    virtual size_t GetRawIndex(IWindowMenuItem &item) = 0; // returns count if item is not member of container

    virtual ~IWinMenuContainer() = default;

    struct Node {
        IWindowMenuItem *item;
        size_t raw_index;
        size_t visible_index;

        bool HasValue() { return item; }
        static constexpr Node Empty() { return { nullptr, 0, 0 }; }
    };

    Node FindFirstVisible() {
        for (size_t i = 0; i < GetCount(); ++i) {
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

    Node FindNextVisible(Node prev) {
        if (!prev.HasValue())
            return Node::Empty();

        for (size_t i = prev.raw_index + 1; i < GetCount(); ++i) {
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

    virtual IWindowMenuItem *GetItemByVisibleIndex(size_t pos) {
        for (Node i = FindFirstVisible(); i.HasValue(); i = FindNextVisible(i)) {
            if (i.visible_index == pos)
                return i.item; // found it
        }
        return nullptr;
    }

    virtual std::optional<size_t> GetVisibleIndex(IWindowMenuItem &item) {
        for (Node i = FindFirstVisible(); i.HasValue(); i = FindNextVisible(i)) {
            if (i.item == &item)
                return i.visible_index; // found it
        }
        return std::nullopt;
    }

    IWindowMenuItem *GetVisibleItemWithOffset(IWindowMenuItem &item, int offset) {
        std::optional<size_t> index = GetVisibleIndex(item);
        if (!index)
            return nullptr;
        int new_index = int(*index) + offset;
        if (new_index < 0)
            return nullptr;
        return GetItemByVisibleIndex(new_index);
    }

    IWindowMenuItem *GetPreviousVisibleItem(IWindowMenuItem &item) {
        return GetVisibleItemWithOffset(item, -1);
    }

    IWindowMenuItem *GetNextVisibleItem(IWindowMenuItem &item) {
        return GetVisibleItemWithOffset(item, 1);
    }
};
