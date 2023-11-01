/**
 * @file i_window_menu_item.hpp
 * @brief Parent of all menu items like label or spinner
 */
#pragma once

#include "GuiDefaults.hpp"
#include <algorithm>
#include <array>
#include "display_helper.h"
#include "super.hpp"
#include "i18n.h"
#include "i_window_menu.hpp" //needed invalidate for click
#include "text_roll.hpp"
#include <utility_extensions.hpp>

// IWindowMenuItem
// todo make version with constant label
// layouts
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
// IWindowMenuItem
class IWindowMenuItem {

public:
    struct ColorScheme {
        struct ColorPair {
            color_t focused;
            color_t unfocused;
        };

        struct ROpPair {
            ropfn focused;
            ropfn unfocused;
        };

        ColorPair text;
        ColorPair back;
        ROpPair rop;
    };

    enum class IconPosition : uint8_t {
        left,
        right,
        replaces_extends,
    };

    /**
     * @brief print extension the same way as label
     * == bolder and whiter than normal extension
     */
    enum class ExtensionLikeLabel : bool {
        no,
        yes
    };

    /// Minimum width of the item extension touch rect
    static constexpr int minimum_touch_extension_area_width = 96;

public:
    /// Returns whether the item is currently in edit mode
    bool is_edited() const;

    /// If set == true, moves focus to the current element and sets it for editing (and cancels focus/editing on the previous one, if any).
    /// If set == false, stops editing mode for the current element or does nothing
    /// Returns if the action was successful
    bool set_is_edited(bool set);

    /// Returns if the action was successful
    inline bool toggle_edit_mode() {
        return set_is_edited(!is_edited());
    }

    static IWindowMenuItem *edited_item();

    /// Focus = visually selected
    bool is_focused() const;

    /// Legacy deprecated alternative to is_focused
    inline bool IsFocused() const {
        return is_focused();
    }

    /// If set == true, moves focus to the current element (and cancels focus/editing on the previous one, if any).
    /// If set == false, unsets the focus from the current element or does nothing
    /// Returns whether the operation was successfull
    bool set_is_focused(bool set);

    /// Moves focus to the current menu item
    inline bool move_focus() {
        return move_focus(this);
    }

    /// Moves focus to the specified menu item (which can be null)
    static bool move_focus(IWindowMenuItem *target);

    inline bool clear_focus() {
        return set_is_focused(false);
    }

    static IWindowMenuItem *focused_item();

protected:
    /// In some situations, one might want to prevent the element from being unselected.
    /// Returning false from this function does that.
    /// This function is not called during the object destruction.
    virtual bool try_exit_edit_mode() { return true; }

protected:
    // could me moved to gui defaults
    static constexpr Rect16::Width_t expand_icon_width = 16;
    static constexpr Rect16::Width_t icon_width = 16;

private:
    font_t *label_font = GuiDefaults::FontMenuItems;
    string_view_utf8 label;
    txtroll_t roll;

    uint8_t hidden : 2;
    is_enabled_t enabled : 1;
    show_disabled_extension_t show_disabled_extension : 1 = show_disabled_extension_t::yes; // Hide disabled menu_items's extension

protected:
    ExtensionLikeLabel has_extension_like_label : 1 = ExtensionLikeLabel::no; // currently has meaning only for menu item info, but might have meaning for other types as well
    uint16_t extension_width : 10;
    bool invalid_icon : 1 = true;
    bool invalid_label : 1 = true;
    bool invalid_extension : 1 = true;
    IconPosition icon_position : 2 { IconPosition::left };
    const img::Resource *id_icon;
    const ColorScheme *clr_scheme { nullptr };

    Rect16 getIconRect(Rect16 rect) const;
    Rect16 getLabelRect(Rect16 rect) const;
    Rect16 getExtensionRect(Rect16 rect) const;
    bool is_touch_in_extension_rect(IWindowMenu &window_menu, point_ui16_t relative_touch_point) const;

    virtual void printIcon(Rect16 icon_rect, ropfn raster_op, color_t color_back) const; // must be virtual, because pictures of flags are drawn differently
    virtual void printExtension(Rect16 extension_rect, color_t color_text, color_t color_back, ropfn raster_op) const; // things behind rect
    virtual void click(IWindowMenu &window_menu) = 0;
    virtual void touch(IWindowMenu &window_menu, point_ui16_t relative_touch_point);
    virtual invalidate_t change(int /*dif*/) { return invalidate_t::no; }

    void setLabelFont(font_t *src);
    font_t *getLabelFont() const;

    void reInitRoll(Rect16 rect);
    void deInitRoll();
    color_t GetTextColor() const;
    color_t GetBackColor() const;

    void showDevOnly() {
        if (hidden != (uint8_t)is_hidden_t::dev) {
            hidden = (uint8_t)is_hidden_t::dev;
            Invalidate();
        }
    }

public:
    IWindowMenuItem(string_view_utf8 label, const img::Resource *id_icon = nullptr, is_enabled_t enabled = is_enabled_t::yes, is_hidden_t hidden = is_hidden_t::no, expands_t expands = expands_t::no);
    IWindowMenuItem(string_view_utf8 label, Rect16::Width_t extension_width_, const img::Resource *id_icon = nullptr, is_enabled_t enabled = is_enabled_t::yes, is_hidden_t hidden = is_hidden_t::no);
    virtual ~IWindowMenuItem();

    void Enable() {
        if (enabled != is_enabled_t::yes) {
            enabled = is_enabled_t::yes;
            Invalidate();
        }
    }
    void Disable() {
        // cannot disable focused item
        if (!is_focused() && enabled != is_enabled_t::no) {
            enabled = is_enabled_t::no;
            Invalidate();
        }
    }

    void hide() {
        hidden = (uint8_t)is_hidden_t::yes;
    }

    void show() {
        if (hidden != (uint8_t)is_hidden_t::no) {
            hidden = (uint8_t)is_hidden_t::no;
            Invalidate();
        }
    }

    void ShowDisabledExtension() {
        if (show_disabled_extension != show_disabled_extension_t::yes) {
            show_disabled_extension = show_disabled_extension_t::yes;
            Invalidate();
        }
    }
    void DontShowDisabledExtension() {
        if (show_disabled_extension != show_disabled_extension_t::no) {
            show_disabled_extension = show_disabled_extension_t::no;
            Invalidate();
        }
    }

    bool IsEnabled() const { return enabled == is_enabled_t::yes; } // This translates to 'shadow' in window_t's derived classes (remains focusable but cant be executed)
    bool DoesShowDisabledExtension() const { return show_disabled_extension == show_disabled_extension_t::yes; }

    bool IsHidden() const;
    bool IsDevOnly() const;

    void SetIconId(const img::Resource *id) {
        id_icon = id;
        InValidateIcon();
    }
    const img::Resource *GetIconId() const { return id_icon; }
    void SetLabel(string_view_utf8 text);
    /// @returns the label translated via gettext
    /// Use this function when you want to get the actual translated text
    /// to be displayed to the user based on his language settings.
    inline string_view_utf8 GetLabel() const { return label; }

    void Print(Rect16 rect);
    void printRoundCorners(Rect16 rect, color_t front, color_t back) const;
    void printOverRoundCorners(Rect16 rect, uint8_t left_width, uint8_t right_width, color_t color_back) const;

    inline bool Increment(uint8_t dif) { return Change(dif); }
    inline bool Decrement(uint8_t dif) { return Change(-int(dif)); }
    bool Change(int dif); // returns if changed
    void Click(IWindowMenu &window_menu);
    void Touch(IWindowMenu &window_menu, point_ui16_t relative_touch_point);
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

    void set_color_scheme(const ColorScheme *scheme);
    void reset_color_scheme();

    void set_icon_position(const IconPosition position);
    IconPosition get_icon_position() const;

    virtual void Loop() {}; // automatically called by menu

    // some friend classes to be able to access / private hide/show methods
    // those methods must not be public, because their usage will break menu!!!
    friend class IWinMenuContainer;
    friend class window_file_list_t;
};
