/**
 * @file i_window_menu_container.hpp
 * @brief abstract container for menu items, also handling visibility and focus.
 * Does not handle visible number of items on screen
 */

#pragma once

#include <stdint.h>
#include <optional>
#include "WindowMenuItems.hpp"

// TODO
// I want same methods for IWinMenuContainer as std::array<IWindowMenuItem *, N> .. need to add iterators

class IWinMenuContainer {
public:
    struct Node {
        IWindowMenuItem *item;
        size_t raw_index;
        size_t visible_index;

        bool HasValue() { return item; }
        static constexpr Node Empty() { return { nullptr, 0, 0 }; }
    };

private:
    IWindowMenuItem *currently_focused = nullptr; // pointer to item makes shorter code than index

    Node findRaw(uint8_t raw_index) const;

public:
    virtual size_t GetRawCount() const = 0;
    virtual IWindowMenuItem *GetItemByRawIndex(size_t pos) const = 0;
    virtual size_t GetRawIndex(IWindowMenuItem &item) const = 0; // returns count if item is not member of container

    virtual ~IWinMenuContainer() = default;

    Node FindFirstVisible() const;
    Node FindNextVisible(Node prev) const;
    IWindowMenuItem *GetItemByVisibleIndex(size_t pos) const;
    std::optional<size_t> GetVisibleIndex(IWindowMenuItem &item) const;
    size_t GetVisibleCount() const;
    IWindowMenuItem *GetVisibleItemWithOffset(IWindowMenuItem &item, int offset) const;

    IWindowMenuItem *GetPreviousVisibleItem(IWindowMenuItem &item) const {
        return GetVisibleItemWithOffset(item, -1);
    }

    IWindowMenuItem *GetNextVisibleItem(IWindowMenuItem &item) const {
        return GetVisibleItemWithOffset(item, 1);
    }

    bool SetIndex(uint8_t visible_index);
    IWindowMenuItem *GetFocused() const { return currently_focused; }
    std::optional<size_t> GetFocusedIndex() const;
    bool Show(IWindowMenuItem &item);
    bool Hide(IWindowMenuItem &item);
    bool SwapVisibility(IWindowMenuItem &item0, IWindowMenuItem &item1);
};
