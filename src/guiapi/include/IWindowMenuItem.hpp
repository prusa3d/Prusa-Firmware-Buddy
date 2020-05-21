#pragma once

#include <stdint.h>
#include <array>
#include "guitypes.h"
#include "Iwindow_menu.hpp" //needed for window settings like rect, padding ...

#pragma pack(push, 1)

//todo make version with constant label
class IWindowMenuItem {
    //protected:
    //   Iwindow_menu_t &window_menu;

private:
    std::array<char, 23> label;
    bool hidden : 1;
    bool enabled : 1;
    bool focused : 1;

protected:
    bool selected : 1; //should be in child, but is here because of size optimalization
private:
    uint16_t id_icon : 10;

    void printIcon(Iwindow_menu_t &window_menu, rect_ui16_t &rect, uint8_t swap) const;

protected:
    virtual void printText(Iwindow_menu_t &window_menu, rect_ui16_t rect, color_t color_text, color_t color_back, uint8_t swap) const;

public:
    IWindowMenuItem(const char *label, uint16_t id_icon, bool enabled = true, bool hidden = false);

    void Enable() { enabled = true; }
    void Disable() { enabled = false; }
    bool IsEnabled() const { return enabled; }
    void SetHidden() { hidden = true; }
    void SetNotHidden() { hidden = false; }
    bool IsHidden() const { return hidden; }
    void SetFocus() { focused = true; }
    void ClrFocus() { focused = false; }
    bool IsFocused() const { return focused; }
    void SetIconId(uint16_t id) { id_icon = id; }
    uint16_t GetIconId() const { return id_icon; }
    void SetLabel(const char *text);
    const char *GetLabel() const;

    void Print(Iwindow_menu_t &window_menu, rect_ui16_t rect) const;

    bool IsSelected() const { return selected; }
    bool Change(int dif) { return dif >= 0 ? Incement(dif) : Decrement(-dif); }
    virtual bool Incement(uint8_t dif) = 0;
    virtual bool Decrement(uint8_t dif) = 0;
    virtual void Click(Iwindow_menu_t &window_menu);

    virtual ~IWindowMenuItem() {}
};
#pragma pack(pop)
