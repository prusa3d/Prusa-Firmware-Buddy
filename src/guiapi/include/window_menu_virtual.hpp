#pragma once

#include <i_window_menu.hpp>
#include <i_window_menu_item.hpp>
#include <utils/storage/inplace_any.hpp>

/// Base class of WindowMenuVirtual (read the desc there)
class WindowMenuVirtualBase : public IWindowMenu {

public:
    /// Whether we should close the screen as return behavior (on swipe left/right)
    enum class CloseScreenReturnBehavior {
        /// No - The window will handle the SWIPE_LEFT/RIGHT differently
        no,

        /// Yes - call Screens::close_screen_or_dialog() on SWIPE_LEFT/RIGHT
        yes,
    };

    static constexpr uint8_t default_item_buffer_size = GuiDefaults::ScreenHeight / IWindowMenu::default_item_height;

public:
    WindowMenuVirtualBase(window_t *parent, Rect16 rect, CloseScreenReturnBehavior close_screen_return_behavior, uint8_t item_buffer_size)
        : IWindowMenu(parent, rect)
        , close_screen_return_behavior_(close_screen_return_behavior)
        , item_buffer_size(item_buffer_size) {}

public:
    IWindowMenuItem *item_at(int index) final;

    std::optional<int> item_index(const IWindowMenuItem *item) const final;

    void set_scroll_offset(int set) override;

protected:
    /// Calls setup_item for the individual slots.
    /// Call this:
    /// * If the item count changed
    /// * If any item type changed
    /// * In the constructor of the most derived class
    void setup_items();

protected:
    virtual IWindowMenuItem *item_at_buffer_slot(int buffer_slot) = 0;
    virtual void setup_buffer_slot(int buffer_slot, std::optional<int> index) = 0;

protected:
    void windowEvent(window_t *sender, GUI_event_t event, void *param) override;
    void screenEvent(window_t *sender, GUI_event_t event, void *param) override;

private:
    std::optional<int> buffer_slot_index(int buffer_slot, int scroll_offset) const;

private:
    CloseScreenReturnBehavior close_screen_return_behavior_;

    const uint8_t item_buffer_size;

    /// Whether \p setup_items() was ever called
    bool items_set_up_ = false;
};

/// WindowMenu implementation that only keeps items that are currently on the screen in the memory.
/// This allows dynamically sized menus (for example file or wi-fi list).
/// User of this class only needs to override \p item_count and \p setup_item functions.
template <uint8_t item_buffer_size_, size_t max_item_sizeof_>
class WindowMenuVirtualSized : public WindowMenuVirtualBase {

public:
    using ItemVariant = InplaceAny<max_item_sizeof_, std::max_align_t, IWindowMenuItem>;
    static constexpr auto item_buffer_size = item_buffer_size_;

public:
    WindowMenuVirtualSized(window_t *parent, Rect16 rect, CloseScreenReturnBehavior close_screen_return_behavior)
        : WindowMenuVirtualBase(parent, rect, close_screen_return_behavior, item_buffer_size) {
    }

protected:
    /// Sets up the provided item for a given index:
    /// - Emplace the correct item type
    /// - Set up the item text and such
    virtual void setup_item(ItemVariant &variant, int index) = 0;

protected:
    void setup_buffer_slot(int buffer_slot, std::optional<int> index) final {
        auto &variant = item_buffer_[buffer_slot];
        if (index.has_value()) {
            setup_item(variant, *index);
        } else {
            variant.reset();
        }
    }

    IWindowMenuItem *item_at_buffer_slot(int buffer_slot) final {
        return item_buffer_[buffer_slot].get_base_if();
    }

    /// \returns item instance variants that are currently in the buffer
    inline auto &buffered_items() {
        return item_buffer_;
    }

private:
    std::array<ItemVariant, item_buffer_size> item_buffer_;
};

using WindowMenuVirtual = WindowMenuVirtualSized<WindowMenuVirtualBase::default_item_buffer_size, 160>;
