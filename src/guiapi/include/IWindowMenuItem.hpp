#pragma once

#include <stdint.h>
#include <array>
#include "guitypes.h"
#include "display_helper.h"
#include "Iwindow_menu.hpp" //needed for window settings like rect, padding ...

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
    bool selected : 1; //should be in child, but is here because of size optimization
private:
    uint16_t id_icon : 10;
    txtroll_t roll;

    void printIcon(Iwindow_menu_t &window_menu, rect_ui16_t rect, uint8_t swap, color_t color_back) const;

protected:
    void printLabel_into_rect(rect_ui16_t rolling_rect, color_t color_text, color_t color_back, const font_t *font, padding_ui8_t padding, uint8_t alignment) const;
    virtual void printText(Iwindow_menu_t &window_menu, rect_ui16_t rect, color_t color_text, color_t color_back, uint8_t swap) const;
    virtual void click(Iwindow_menu_t &window_menu) = 0;
    virtual rect_ui16_t getRollingRect(Iwindow_menu_t &window_menu, rect_ui16_t rect) const;
    rect_ui16_t getIconRect(Iwindow_menu_t &window_menu, rect_ui16_t rect) const;

public:
    IWindowMenuItem(const char *label, uint16_t id_icon, bool enabled = true, bool hidden = false);

    void Enable() { enabled = true; }
    void Disable() { enabled = false; }
    bool IsEnabled() const { return enabled; }
    void SetHidden() { hidden = true; }
    void SetNotHidden() { hidden = false; }
    bool IsHidden() const { return hidden; }
    void SetFocus();
    void ClrFocus() { focused = false; }
    bool IsFocused() const { return focused; }
    void SetIconId(uint16_t id) { id_icon = id; }
    uint16_t GetIconId() const { return id_icon; }
    void SetLabel(const char *text);
    const char *GetLabel() const;

    void Print(Iwindow_menu_t &window_menu, rect_ui16_t rect) const;

    bool IsSelected() const { return selected; }
    virtual bool Change(int dif) = 0;
    bool Increment(uint8_t dif) { return Change(dif); }
    bool Decrement(uint8_t dif) { return Change(-int(dif)); }
    void Click(Iwindow_menu_t &window_menu);
    void Roll(Iwindow_menu_t &window_menu);
    void RollInit(Iwindow_menu_t &window_menu, rect_ui16_t rect);
    bool RollNeedInit() { return roll.setup == TXTROLL_SETUP_INIT; }
    virtual ~IWindowMenuItem() = default;
};
