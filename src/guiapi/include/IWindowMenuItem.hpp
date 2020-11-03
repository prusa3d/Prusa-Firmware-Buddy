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
    is_hidden_t hidden : 1;
    is_enabled_t enabled : 1;
    is_focused_t focused : 1;

protected:
    is_selected_t selected : 1; //should be in child, but is here because of size optimization
    expands_t expands : 1;      // determines if Label expands to another screen (with a click)
    uint16_t id_icon : 10;

private:
    txtroll_t roll;

protected:
    virtual void printIcon(IWindowMenu &window_menu, Rect16 rect, uint8_t swap, color_t color_back) const;
    void printLabel_into_rect(Rect16 rolling_rect, color_t color_text, color_t color_back, const font_t *font, padding_ui8_t padding, uint8_t alignment) const;
    virtual void printItem(IWindowMenu &window_menu, Rect16 rect, color_t color_text, color_t color_back, uint8_t swap) const;
    virtual void click(IWindowMenu &window_menu) = 0;
    virtual Rect16 getRollingRect(IWindowMenu &window_menu, Rect16 rect) const;
    virtual std::array<Rect16, 2> getMenuRects(IWindowMenu &window_menu, Rect16 rect) const = 0;
    static Rect16 getCustomRect(IWindowMenu &window_menu, Rect16 base_rect, uint16_t custom_rect_width);
    static Rect16 getIconRect(IWindowMenu &window_menu, Rect16 rect);
    void reInitRoll(IWindowMenu &window_menu, Rect16 rect);

public:
    IWindowMenuItem(string_view_utf8 label, uint16_t id_icon, is_enabled_t enabled = is_enabled_t::yes, is_hidden_t hidden = is_hidden_t::no, expands_t expands = expands_t::no);

    void Enable() { enabled = is_enabled_t::yes; }
    void Disable() { enabled = is_enabled_t::no; }
    bool IsEnabled() const { return enabled == is_enabled_t::yes; }
    void Hide() { hidden = is_hidden_t::yes; }
    void Show() { hidden = is_hidden_t::no; }
    bool IsHidden() const { return hidden == is_hidden_t::yes; }
    void SetFocus();
    void ClrFocus();
    bool IsFocused() const { return focused == is_focused_t::yes; }
    void SetIconId(uint16_t id) { id_icon = id; }
    uint16_t GetIconId() const { return id_icon; }
    void SetLabel(string_view_utf8 text);
    /// @returns the label translated via gettext
    /// Use this function when you want to get the actual translated text
    /// to be displayed to the user based on his language settings.
    string_view_utf8 GetLabel() const;

    void Print(IWindowMenu &window_menu, Rect16 rect) const;

    bool IsSelected() const { return selected == is_selected_t::yes; }
    virtual bool Change(int dif) = 0;
    bool Increment(uint8_t dif) { return Change(dif); }
    bool Decrement(uint8_t dif) { return Change(-int(dif)); }
    void Click(IWindowMenu &window_menu);
    virtual void InitRollIfNeeded(IWindowMenu &window_menu, Rect16 rect) = 0;
    invalidate_t Roll();
    virtual ~IWindowMenuItem() = default;
};
