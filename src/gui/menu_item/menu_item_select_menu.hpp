#pragma once

#include <span>

#include <i_window_menu_item.hpp>
#include <gui/dialogs/IDialog.hpp>
#include <window_menu_virtual.hpp>
#include <window_menu_callback_item.hpp>
#include <window_header.hpp>
#include <WindowMenuItems.hpp>

/// Menu item where user selects a value from a list of items presented as a menu dialog
class MenuItemSelectMenu : public IWindowMenuItem {

public:
    static constexpr size_t value_buffer_size = 32;
    static constexpr Font value_font = GuiDefaults::FontMenuItems;

    MenuItemSelectMenu(const string_view_utf8 &label);

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

    /// Stores text representation of item at \param index to \param buffer
    virtual void build_item_text(int index, const std::span<char> &buffer) const = 0;

protected:
    /// Called when the current item is changed by the user
    /// \returns whether the item selection is valid.
    /// Does not change the select value if \p false is returned
    virtual bool on_item_selected([[maybe_unused]] int old_index, [[maybe_unused]] int new_index) { return true; }

protected:
    void printExtension(Rect16 extension_rect, Color color_text, Color color_back, ropfn raster_op) const override;

    void click(IWindowMenu &) override;

private:
    int current_item_ = -1;
    std::array<char, value_buffer_size> value_text_ { '\0' };
};
