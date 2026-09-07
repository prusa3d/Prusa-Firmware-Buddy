#pragma once

#include <i_window_menu_item.hpp>

/// Menu item where user selects a value from a list of items presented as a menu dialog
class MenuItemSelectMenu : public IWindowMenuItem {

public:
    enum class Behavior : uint8_t {
        /// On click, a submenu is opened, listing all available items
        submenu,

        /// On click, a next item is selected, wrapping around
        /// !!! Not suitable for menus where there's more than two items
        /// !!! or where its not obvious what the items are
        quick_cycle,

        /// Configures the menu item to work as a "popup to select action" only
        /// - On click, pops up the submenu, same as the submenu behavior
        /// - The current_item() values becomes irrelevant (and should stay -1 all the time)
        /// - Instead of extension, the ">" arrow will be rendered (bcs current_item is irrelevant)
        /// - The on_item_selected still works as expected
        select_only,

        _last = select_only,
    };

    static constexpr Font value_font = GuiDefaults::FontMenuItems;

    using ItemTextParams = StringViewUtf8Parameters<16>;

    MenuItemSelectMenu(const string_view_utf8 &label);

    void set_behavior(Behavior set);

    /// \returns currently selected item
    int current_item() const {
        return current_item_;
    }

    /// Changes the currently selected item to \param set
    void set_current_item(int set);

    /// Changes the currently selected item to \param set. Always invalidates and rebuilds.
    void force_set_current_item(int set);

    /// \returns number of items in the list
    virtual int item_count() const = 0;

    /// Builds text for item @p index and returns it. Can use @param params to store supplementary data.
    virtual string_view_utf8 build_item_text(int index, ItemTextParams &params) const = 0;

protected:
    struct OnItemSelectedArgs {
        int old_index;
        int new_index;
        IWindowMenu &menu;
    };

    /// Called when the current item is changed by the user
    /// \returns whether the item selection is valid.
    /// Does not change the select value if \p false is returned
    virtual bool on_item_selected([[maybe_unused]] const OnItemSelectedArgs &args) {
        return true;
    }

protected:
    /// Resolves the value text color (base highlights the focused & enabled item).
    /// @param base_color color used when the item is not focused & enabled
    virtual Color resolved_value_text_color(Color base_color) const;
    void printExtension(Rect16 extension_rect, Color color_text, Color color_back, ropfn raster_op) const override;

    void click(IWindowMenu &) override;

private:
    int current_item_ = -1;
    string_view_utf8 value_;
    ItemTextParams value_params_;
    Behavior behavior_ = Behavior::submenu;
};
