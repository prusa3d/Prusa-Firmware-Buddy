/**
 * @file i_window_menu_item.hpp
 * @brief Parent of all menu items like label or spinner
 */
#pragma once

#include <guiconfig/GuiDefaults.hpp>
#include <algorithm>
#include <array>
#include "display_helper.h"
#include "i18n.h"
#include "i_window_menu.hpp" //needed invalidate for click
#include "text_roll.hpp"
#include <utility_extensions.hpp>
#include <gui/event/gui_event.hpp>

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

class WindowMenuItemEventContext : public GuiEventContext {

public:
    inline WindowMenuItemEventContext(GuiEventType auto &&event, IWindowMenu *menu)
        : GuiEventContext(event)
        , menu(menu) {
    }

public:
    // TODO: Get rid of this completely, currently here only to keep compatibility with the old API
    /// Menu of the item the event is called for. Might be not known.
    IWindowMenu *const menu = nullptr;
};

/*****************************************************************************/
// IWindowMenuItem
class IWindowMenuItem {

public:
    struct ColorScheme {
        struct ColorPair {
            Color focused;
            Color unfocused;
        };

        struct ROpPair {
            ropfn focused = { .invert = is_inverted::yes };
            ropfn unfocused = {};
        };

        ColorPair text = {
            .focused = GuiDefaults::MenuColorBack,
            .unfocused = GuiDefaults::MenuColorText,
        };
        ColorPair back = {
            .focused = GuiDefaults::MenuColorFocusedBack,
            .unfocused = GuiDefaults::MenuColorBack,
        };
        ROpPair rop = {};
    };

    static const ColorScheme color_scheme_title;

    enum class IconPosition : uint8_t {
        left,
        right,
        replaces_extends,
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
    // could me moved to gui defaults
    static constexpr Rect16::Width_t expand_icon_width = 16;
    static constexpr Rect16::Width_t icon_width = 16;

private:
    Font label_font = GuiDefaults::FontMenuItems;
    string_view_utf8 label;

    uint8_t hidden : 2;
    is_enabled_t enabled : 1;
    show_disabled_extension_t show_disabled_extension : 1 = show_disabled_extension_t::yes; // Hide disabled menu_items's extension

protected:
    uint16_t extension_width : 10;
    /// Marks this menu item as returning.
    /// TOUCH_SWIPE_LEFT gesture tries to find an item with this flag in the menu and execute it.
    bool has_return_behavior_ : 1 = false;

    /// If set, touch event generates the click event only when the touch happens in the extension rect by default
    bool touch_extension_only_ : 1 = false;

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

    virtual void printIcon(Rect16 icon_rect, ropfn raster_op, Color color_back) const; // must be virtual, because pictures of flags are drawn differently
    virtual void printExtension(Rect16 extension_rect, Color color_text, Color color_back, ropfn raster_op) const; // things behind rect
    virtual void click([[maybe_unused]] IWindowMenu &window_menu) {};
    virtual invalidate_t change(int /*dif*/) { return invalidate_t::no; }
    virtual void event(WindowMenuItemEventContext &);

    void setLabelFont(Font);
    Font getLabelFont() const;

    Color GetTextColor() const;
    Color GetBackColor() const;

    void showDevOnly() {
        if (hidden != (uint8_t)is_hidden_t::dev) {
            hidden = (uint8_t)is_hidden_t::dev;
            Invalidate();
        }
    }

    // Make the destructor protected. It is not virtual to save flash (because of vtables), so we want to prevent someone accidentally calling it "dynamically" on a base class.
    ~IWindowMenuItem();

public:
    IWindowMenuItem(const string_view_utf8 &label, const img::Resource *id_icon = nullptr, is_enabled_t enabled = is_enabled_t::yes, is_hidden_t hidden = is_hidden_t::no, expands_t expands = expands_t::no);
    IWindowMenuItem(const string_view_utf8 &label, Rect16::Width_t extension_width_, const img::Resource *id_icon = nullptr, is_enabled_t enabled = is_enabled_t::yes, is_hidden_t hidden = is_hidden_t::no);
    IWindowMenuItem(const IWindowMenuItem &) = delete;

    bool IsEnabled() const { return enabled == is_enabled_t::yes; } // This translates to 'shadow' in window_t's derived classes (remains focusable but cant be executed)
    void set_is_enabled(bool set = true);

    /// Deprecated. Use set_enabled
    inline void Enable() {
        set_is_enabled(true);
    }

    /// Deprecated. Use set_enabled
    inline void Disable() {
        set_is_enabled(false);
    }

    bool DoesShowDisabledExtension() const { return show_disabled_extension == show_disabled_extension_t::yes; }

    /// Sets whether the item should show the extension (value) when disabled
    void set_show_disabled_extension(bool set);

    /// Deprecated. Use set_show_disabled_extension
    inline void ShowDisabledExtension() {
        set_show_disabled_extension(true);
    }

    /// Deprecated. Use set_show_disabled_extension
    inline void DontShowDisabledExtension() {
        set_show_disabled_extension(false);
    }

    inline void set_is_hidden(is_hidden_t set) {
        const bool wasHidden = IsHidden();
        hidden = static_cast<uint8_t>(set);

        if (!IsHidden() && wasHidden) {
            Invalidate();
        }
    }
    inline void set_is_hidden(bool set = true) {
        set_is_hidden(set ? is_hidden_t::yes : is_hidden_t::no);
    }

    void hide() {
        set_is_hidden(true);
    }

    void show() {
        set_is_hidden(false);
    }

    bool IsHidden() const;
    bool IsDevOnly() const;

    void SetIconId(const img::Resource *id);
    void SetLabel(const string_view_utf8 &text);
    /// @returns the label translated via gettext
    /// Use this function when you want to get the actual translated text
    /// to be displayed to the user based on his language settings.
    inline const string_view_utf8 &GetLabel() const { return label; }

    void Print(Rect16 rect);
    void printRoundCorners(Rect16 rect, Color front, Color back) const;
    void printOverRoundCorners(Rect16 rect, uint8_t left_width, uint8_t right_width, Color color_back) const;

    inline bool Increment(uint8_t dif) { return Change(dif); }
    inline bool Decrement(uint8_t dif) { return Change(-int(dif)); }
    bool Change(int dif); // returns if changed
    void Click(IWindowMenu &window_menu);
    void Touch(IWindowMenu &window_menu, point_ui16_t relative_touch_point);

    /// Handles text roll on the focused item
    static void handle_roll();

    /// Reset text roll of the focused item
    static void reset_roll();

    bool IsInvalid() const;
    bool IsIconInvalid() const;
    bool IsLabelInvalid() const;
    bool IsExtensionInvalid() const;
    void Validate();
    void Invalidate();
    void InValidateIcon();
    void InValidateLabel();
    void InValidateExtension();

    inline bool has_return_behavior() const {
        return has_return_behavior_;
    }

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

/// Final subclass of IWindowMenuItem to get around the protected IWindowMenuItem destructor
class WindowMenuItem : public IWindowMenuItem {

public:
    using IWindowMenuItem::IWindowMenuItem;
};

template <typename T>
concept UpdatableMenuItem = requires(T a) {
    requires std::is_base_of_v<IWindowMenuItem, T>;

    { a.update() };
};
