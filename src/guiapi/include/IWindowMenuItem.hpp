#pragma once

#include <stdint.h>
#include <array>
#include "guitypes.h"
#include "window_menu.hpp"

#pragma pack(push, 1)

class IWindowMenuItem {
protected:
    window_menu_t &window_menu;

private:
    std::array<char, 23> label;
    bool hidden : 1;
    bool enabled : 1;
    bool focused : 1;
    uint16_t id_icon : 10;

    void printIcon(rect_ui16_t &rect, uint8_t swap) const;

protected:
    virtual void printText(rect_ui16_t rect, color_t color_text, color_t color_back, uint8_t swap) const;

public:
    IWindowMenuItem(window_menu_t &window_menu, const char *label, uint16_t id_icon, bool enabled = true, bool hidden = false);

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

    void Print(rect_ui16_t rect) const;

    virtual int OnClick() = 0;
    virtual bool Change(int dif) = 0;

    virtual ~IWindowMenuItem() {}
};
#pragma pack(pop)
