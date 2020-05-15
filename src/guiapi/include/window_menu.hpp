#pragma once

#include "window_menu.h"
#include "Iwindow_menu.hpp"
#include "IWinMenuContainer.hpp"
#include <stdint.h>

#pragma pack(push, 1)

class window_menu_t : public Iwindow_menu_t {
    uint8_t index;

public:
    uint8_t top_index;
    IWinMenuContainer *pContainer;
    bool SetIndex(uint8_t index); //must check container
    void Incement(int dif);
    void Decrement(int dif) { Incement(-dif); }
    uint8_t GetIndex() const { return index; }
    uint8_t GetCount() const;
    IWindowMenuItem *GetItem(uint8_t index);
    IWindowMenuItem *GetActiveItem();
};

#pragma pack(pop)
