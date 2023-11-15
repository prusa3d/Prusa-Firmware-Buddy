/**
 * @file i_window_menu.hpp
 * @brief abstract menu
 */

#pragma once

#include <stdint.h>

#include "window.hpp"

class IWindowMenu : public AddSuperWindow<window_t> {

protected:
    static constexpr uint8_t font_h_ = 19; // Fonts are not constexpr
    static constexpr uint16_t item_height_ = font_h_ + GuiDefaults::MenuPaddingItems.top + GuiDefaults::MenuPaddingItems.bottom;

public:
    enum class YNPlaySound {
        no,
        yes,
    };

    enum class PageScrollDirection {
        up,
        down
    };

public:
    /// Total item count in the menu
    virtual int item_count() const = 0;

    /// \returns how many items fit on the screen
    inline int max_items_on_screen_count() const {
        return max_items_on_screen_count_;
    }

    /// \returns how many items there are currently visible on the scren
    inline int current_items_on_screen_count() const {
        return std::min(item_count() - scroll_offset(), max_items_on_screen_count());
    }

    /// \returns height of a single item (in pixels)
    static constexpr inline uint16_t item_height() {
        return item_height_;
    }

public: // Scroll related stuff
    /// Index of the first item on the screen (slot 0)
    inline int scroll_offset() const {
        return scroll_offset_;
    }

    /// Maximum allowed scroll index, given the current item_count and max_items_on_screen
    inline int max_scroll_offset() const {
        return std::max(item_count() - max_items_on_screen_count(), 0);
    }

    /// Scrolls the view so that the provided index is on the first slot
    /// Providing invalid $set results in an error.
    virtual void set_scroll_offset(int set);

    /// Scrolls the menu so that the given index is on the screen (if necessary)
    /// \returns if the scroll index changes
    bool ensure_item_on_screen(std::optional<int> index);

    /// Scrolls the menu by one page. Returns true if top_index changed.
    bool scroll_page(PageScrollDirection direction);

public: // Focus related stuff
    /// Returns index of the focused item (if there is an focused item)
    virtual std::optional<int> focused_item_index() const = 0;

    inline std::optional<int> focused_slot() const {
        return index_to_slot(focused_item_index());
    }

    /// Sets $index to be focused.
    /// The implementaiton should also ensure that the focused index is on the screen afterwards
    /// \returns if successful
    virtual bool move_focus_to_index(std::optional<int> index) = 0;

    /// Moves the focus by $amount items. Sets the focus if there is no focus.
    /// \returns if the focus was changed
    bool move_focus_by(int amount, YNPlaySound play_sound);

    /// Tries to move focus as a result of the touch click event (param is to be passed here).
    ///
    /// !!! It is REQUIRED to use this function when children of i_window_menu are handling TOUCH_CLICK on their own,
    /// !!! because the focus move can fail (IWindowMenuItem::try_exit_edit_mode)
    ///
    /// \returns focused index of the clicked item, if the focus move was successful
    std::optional<int> move_focus_touch_click(void *event_param);

    /// Returns whether the container should focus an item on container init.
    /// This is false with enabled touchscreen.
    static bool should_focus_item_on_init();

public:
    /// slot = item on the screen (slot 0 = topmost item on the screen)
    /// Returns rectangle for item on a screen
    Rect16 slot_rect(int slot) const;

    /// Returns slot that corresponds to the given point on the screen
    std::optional<int> slot_at_point(point_ui16_t point) const;

    /// Transforms index (index of the item across all the items) to slot (item index on the screen)
    /// \returns nullopt if the index is outside of valid range or not on the screen
    std::optional<int> index_to_slot(std::optional<int> index) const;

protected:
    IWindowMenu(window_t *parent, Rect16 rect);

protected:
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;

private:
    int max_items_on_screen_count_;

    /// How many items we've scrolled down by
    /// Formwrly known as index_of_first
    int scroll_offset_ = 0;
};
