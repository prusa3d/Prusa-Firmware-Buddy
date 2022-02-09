/**
 * @file IWindowMenuItem.hpp
 * @author Radek Vana
 * @brief Parent of all menu items like label or spinner
 * @date 2020-11-09
 */
#pragma once

#include "GuiDefaults.hpp"
#include <algorithm>
#include <array>
#include "display_helper.h"
#include "super.hpp"
#include "i18n.h"
#include "Iwindow_menu.hpp" //needed invalidate for click
#include "text_roll.hpp"

//IWindowMenuItem
//todo make version with constant label
//layouts
//+-------+-----------------------------+
//| icon  | text                        | label
//+-------+--------------+--------------+
//| icon  | text         | arrow        | label with expand
//+-------+--------------+--------------+
//| icon  | text         | value        | spin
//+-------+--------------+-------+------+
//| icon  | text         | value | unit | spin with units
//+-------+--------------+-------+------+
//| icon  | text         | value        |
//+-------+--------------+--------------+
//| icon  | text         | [value]      | switch with brackets
//+-------+--------------+--------------+

/*****************************************************************************/
//IWindowMenuItem
class IWindowMenuItem {
protected:
    //could me moved to gui defaults
    static constexpr Rect16::Width_t expand_icon_width = 26;
    static constexpr Rect16::Width_t icon_width = 26;

private:
    string_view_utf8 label;
    txtroll_t roll;

    uint8_t hidden : 2;
    is_enabled_t enabled : 1;
    is_focused_t focused : 1;

protected:
    is_selected_t selected : 1; // should be in IWiSpin, but is here because of size optimization
    uint16_t id_icon : 10;
    Rect16::Width_t extension_width; // must be behind bitfields to save 4B RAM per item
    font_t *label_font;

    static Rect16 getCustomRect(Rect16 base_rect, uint16_t custom_rect_width); // general method Returns custom width Rectangle, aligned intersection on the right of the base_rect
    Rect16 getIconRect(Rect16 rect) const;
    Rect16 getLabelRect(Rect16 rect) const;
    Rect16 getExtensionRect(Rect16 rect) const;

    virtual void printIcon(Rect16 icon_rect, ropfn raster_op, color_t color_back) const;                               //must be virtual, because pictures of flags are drawn differently
    virtual void printExtension(Rect16 extension_rect, color_t color_text, color_t color_back, ropfn raster_op) const; //things behind rect
    virtual void click(IWindowMenu &window_menu) = 0;

    void setLabelFont(font_t *src) { label_font = src; }
    font_t *getLabelFont() const { return label_font; }

    void reInitRoll(Rect16 rect);
    color_t GetTextColor() const;
    color_t GetBackColor() const;

public:
    IWindowMenuItem(string_view_utf8 label, uint16_t id_icon = 0, is_enabled_t enabled = is_enabled_t::yes, is_hidden_t hidden = is_hidden_t::no, expands_t expands = expands_t::no, font_t *label_font = GuiDefaults::FontMenuItems);
    IWindowMenuItem(string_view_utf8 label, Rect16::Width_t extension_width_, uint16_t id_icon = 0, is_enabled_t enabled = is_enabled_t::yes, is_hidden_t hidden = is_hidden_t::no, font_t *label_font = GuiDefaults::FontMenuItems);
    virtual ~IWindowMenuItem() = default;
    void Enable() { enabled = is_enabled_t::yes; }
    void Disable() { enabled = is_enabled_t::no; }
    bool IsEnabled() const { return enabled == is_enabled_t::yes; } // This translates to 'shadow' in window_t's derived classes (remains focusable but cant be executed)
    bool IsSelected() const { return selected == is_selected_t::yes; }
    void Hide() { hidden = (uint8_t)is_hidden_t::yes; }
    void Show() { hidden = (uint8_t)is_hidden_t::no; }
    void SetVisibility(bool visible) { hidden = (uint8_t)!visible; }
    void ShowDevOnly() { hidden = (uint8_t)is_hidden_t::dev; }
    bool IsHidden() const;
    void SetFocus();
    void ClrFocus();
    bool IsFocused() const { return focused == is_focused_t::yes; }
    void SetIconId(uint16_t id) { id_icon = id; }
    uint16_t GetIconId() const { return id_icon; }
    inline void SetLabel(string_view_utf8 text) { label = text; }
    /// @returns the label translated via gettext
    /// Use this function when you want to get the actual translated text
    /// to be displayed to the user based on his language settings.
    inline string_view_utf8 GetLabel() const { return label; }

    void Print(Rect16 rect) const;

    inline invalidate_t Increment(uint8_t dif) { return Change(dif); }
    inline invalidate_t Decrement(uint8_t dif) { return Change(-int(dif)); }
    void Click(IWindowMenu &window_menu);
    inline void InitRollIfNeeded(Rect16 rect) { reInitRoll(getLabelRect(rect)); }
    virtual invalidate_t Change(int /*dif*/) { return invalidate_t::no; }
    inline invalidate_t Roll() { return roll.Tick(); }
};
