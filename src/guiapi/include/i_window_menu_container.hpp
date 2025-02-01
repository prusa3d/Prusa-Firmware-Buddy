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
        int raw_index;
        int visible_index;

        bool HasValue() { return item; }
        static constexpr Node Empty() { return { nullptr, 0, 0 }; }
    };

public:
    virtual int GetRawCount() const = 0;
    virtual IWindowMenuItem *GetItemByRawIndex(int pos) const = 0;
    virtual int GetRawIndex(IWindowMenuItem &item) const = 0; // returns count if item is not member of container

    Node FindFirstVisible() const;
    Node FindNextVisible(Node prev) const;
    IWindowMenuItem *GetItemByVisibleIndex(int pos) const;
    std::optional<int> GetVisibleIndex(const IWindowMenuItem &item) const;
    int GetVisibleCount() const;

    bool set_index(int visible_index);
    std::optional<int> GetFocusedIndex() const;
    bool SwapVisibility(IWindowMenuItem &item0, IWindowMenuItem &item1);
};
