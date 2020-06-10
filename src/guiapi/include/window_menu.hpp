#pragma once

#include "window_menu.h"
#include "Iwindow_menu.hpp"
#include "IWinMenuContainer.hpp"
#include <stdint.h>

//todo
//use template instead IWinMenuContainer *pContainer;
//I want same methods for IWinMenuContainer as std::array<IWindowMenuItem *, N>  .. need to add iterators
class window_menu_t : public Iwindow_menu_t {
    uint8_t index;
    void setIndex(uint8_t index); //for ctor (cannot fail)
public:
    window_menu_t(IWinMenuContainer *pContainer, uint8_t index = 0);
    uint8_t top_index;
    IWinMenuContainer *pContainer;
    bool SetIndex(uint8_t index); //must check container
    void Increment(int dif);
    void Decrement(int dif) { Increment(-dif); }
    uint8_t GetIndex() const { return index; }
    uint8_t GetCount() const;
    IWindowMenuItem *GetItem(uint8_t index) const;
    IWindowMenuItem *GetActiveItem();
};
