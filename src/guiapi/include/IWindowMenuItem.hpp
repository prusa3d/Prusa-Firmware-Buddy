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
    uint16_t extension_width : 10; // must be behind bitfields to save 4B RAM per item
    bool invalid_icon : 1;
    bool invalid_label : 1;
    bool invalid_extension : 1;
    font_t *label_font;

    static Rect16 getCustomRect(Rect16 base_rect, uint16_t custom_rect_width); // general method Returns custom width Rectangle, aligned intersection on the right of the base_rect
    Rect16 getIconRect(Rect16 rect) const;
    Rect16 getLabelRect(Rect16 rect) const;
    Rect16 getExtensionRect(Rect16 rect) const;

    virtual void printIcon(Rect16 icon_rect, ropfn raster_op, color_t color_back) const;                               //must be virtual, because pictures of flags are drawn differently
    virtual void printExtension(Rect16 extension_rect, color_t color_text, color_t color_back, ropfn raster_op) const; //things behind rect
    virtual void click(IWindowMenu &window_menu) = 0;
    virtual invalidate_t change(int /*dif*/) { return invalidate_t::no; }

    void setLabelFont(font_t *src) { label_font = src; }
    font_t *getLabelFont() const { return label_font; }

    void reInitRoll(Rect16 rect);
    color_t GetTextColor() const;
    color_t GetBackColor() const;

    void hide() {
        hidden = (uint8_t)is_hidden_t::yes;
    }
    void show() {
        if (hidden != (uint8_t)is_hidden_t::no) {
            hidden = (uint8_t)is_hidden_t::no;
            Invalidate();
        }
    }

    void showDevOnly() {
        if (hidden != (uint8_t)is_hidden_t::dev) {
            hidden = (uint8_t)is_hidden_t::dev;
            Invalidate();
        }
    }

    void setFocus(); // will show hidden
    void clrFocus();

public:
    IWindowMenuItem(string_view_utf8 label, uint16_t id_icon = 0, is_enabled_t enabled = is_enabled_t::yes, is_hidden_t hidden = is_hidden_t::no, expands_t expands = expands_t::no, font_t *label_font = GuiDefaults::FontMenuItems);
    IWindowMenuItem(string_view_utf8 label, Rect16::Width_t extension_width_, uint16_t id_icon = 0, is_enabled_t enabled = is_enabled_t::yes, is_hidden_t hidden = is_hidden_t::no, font_t *label_font = GuiDefaults::FontMenuItems);
    virtual ~IWindowMenuItem() = default;
    void Enable() {
        if (enabled != is_enabled_t::yes) {
            enabled = is_enabled_t::yes;
            Invalidate();
        }
    }
    void Disable() {
        //cannot disable focused item
        if (focused != is_focused_t::yes && enabled != is_enabled_t::no) {
            enabled = is_enabled_t::no;
            Invalidate();
        }
    }
    bool IsEnabled() const { return enabled == is_enabled_t::yes; } // This translates to 'shadow' in window_t's derived classes (remains focusable but cant be executed)
    bool IsSelected() const { return selected == is_selected_t::yes; }

    bool IsHidden() const;

    bool IsFocused() const { return focused == is_focused_t::yes; }
    void SetIconId(uint16_t id) {
        id_icon = id;
        InValidateIcon();
    }
    uint16_t GetIconId() const { return id_icon; }
    void SetLabel(string_view_utf8 text);
    /// @returns the label translated via gettext
    /// Use this function when you want to get the actual translated text
    /// to be displayed to the user based on his language settings.
    inline string_view_utf8 GetLabel() const { return label; }

    void Print(Rect16 rect);

    inline bool Increment(uint8_t dif) { return Change(dif); }
    inline bool Decrement(uint8_t dif) { return Change(-int(dif)); }
    bool Change(int dif); // returns if changed
    void Click(IWindowMenu &window_menu);
    inline void InitRollIfNeeded(Rect16 rect) { reInitRoll(getLabelRect(rect)); }

    void Roll();

    bool IsInvalid() const;
    bool IsIconInvalid() const;
    bool IsLabelInvalid() const;
    bool IsExtensionInvalid() const;
    void Validate();
    void Invalidate();
    void InValidateIcon();
    void InValidateLabel();
    void InValidateExtension();

    virtual void Loop() {}; //automatically called by menu

    // some friend classes to be able to access / private hide/show methods
    // those methods must not be public, because their usage will break menu!!!
    friend class window_menu_t;
    friend class window_file_list_t;
};
