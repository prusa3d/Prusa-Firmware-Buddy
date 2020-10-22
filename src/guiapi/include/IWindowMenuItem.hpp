#pragma once

#include <stdint.h>
#include <array>
#include "guitypes.hpp"
#include "display_helper.h"
#include "Iwindow_menu.hpp" //needed for window settings like rect, padding ...
#include "text_roll.hpp"

//todo make version with constant label
class IWindowMenuItem {
    //protected:
    //   IWindowMenu &window_menu;

private:
    string_view_utf8 label;
    bool hidden : 1;
    bool enabled : 1;
    bool focused : 1;

protected:
    bool selected : 1; //should be in child, but is here because of size optimization
    uint16_t id_icon : 10;

private:
    txtroll_t roll;

protected:
    virtual void printIcon(IWindowMenu &window_menu, Rect16 rect, uint8_t swap, color_t color_back) const;
    void printLabel_into_rect(Rect16 rolling_rect, color_t color_text, color_t color_back, const font_t *font, padding_ui8_t padding, uint8_t alignment) const;
    virtual void printText(IWindowMenu &window_menu, Rect16 rect, color_t color_text, color_t color_back, uint8_t swap) const;
    virtual void click(IWindowMenu &window_menu) = 0;
    virtual Rect16 getRollingRect(IWindowMenu &window_menu, Rect16 rect) const;
    static Rect16 getIconRect(IWindowMenu &window_menu, Rect16 rect);

public:
    IWindowMenuItem(string_view_utf8 label, uint16_t id_icon, bool enabled = true, bool hidden = false);

    void Enable() { enabled = true; }
    void Disable() { enabled = false; }
    bool IsEnabled() const { return enabled; }
    void Hide() { hidden = true; }
    void Show() { hidden = false; }
    bool IsHidden() const { return hidden; }
    void SetFocus();
    void ClrFocus();
    bool IsFocused() const { return focused; }
    void SetIconId(uint16_t id) { id_icon = id; }
    uint16_t GetIconId() const { return id_icon; }
    void SetLabel(string_view_utf8 text);
    /// @returns the label translated via gettext
    /// Use this function when you want to get the actual translated text
    /// to be displayed to the user based on his language settings.
    string_view_utf8 GetLabel() const;

    void Print(IWindowMenu &window_menu, Rect16 rect) const;

    bool IsSelected() const { return selected; }
    virtual bool Change(int dif) = 0;
    bool Increment(uint8_t dif) { return Change(dif); }
    bool Decrement(uint8_t dif) { return Change(-int(dif)); }
    void Click(IWindowMenu &window_menu);
    invalidate_t Roll();
    void InitRollIfNeeded(IWindowMenu &window_menu, Rect16 rect);
    virtual ~IWindowMenuItem() = default;
};
