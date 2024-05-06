#pragma once

#include <array>
#include <variant>
#include <i_window_menu.hpp>
#include <i_window_menu_item.hpp>

/// Base class of WindowMenuVirtual (read the desc there)
class WindowMenuVirtualBase : public IWindowMenu {

public:
    static constexpr int item_buffer_size = GuiDefaults::ScreenHeight / IWindowMenu::item_height();

public:
    WindowMenuVirtualBase(window_t *parent, Rect16 rect)
        : IWindowMenu(parent, rect) {}

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
    virtual void windowEvent(EventLock, window_t *sender, GUI_event_t event, void *param) override;

private:
    std::optional<int> buffer_slot_index(int buffer_slot, int scroll_offset) const;
};

/// WindowMenu implementation that only keeps items that are currently on the screen in the memory.
/// This allows dynamically sized menus (for example file or wi-fi list).
/// \p ItemVariants specifies what IWindowMenu subtypes the menu uses and can spawn.
/// User of this class only needs to override \p item_count and \p setup_item functions.
template <typename... ItemVariants>
class WindowMenuVirtual : public WindowMenuVirtualBase {

public:
    using ItemVariant = std::variant<std::monostate, ItemVariants...>;

public:
    WindowMenuVirtual(window_t *parent, Rect16 rect)
        : WindowMenuVirtualBase(parent, rect) {}

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
            variant.template emplace<std::monostate>();
        }
    }

    IWindowMenuItem *item_at_buffer_slot(int buffer_slot) final {
        constexpr auto visit_f = []<typename T>(T &item) -> IWindowMenuItem * {
            if constexpr (std::is_same_v<T, std::monostate>) {
                return nullptr;
            } else {
                return static_cast<IWindowMenuItem *>(&item);
            }
        };

        return std::visit(visit_f, item_buffer_[buffer_slot]);
    }

private:
    std::array<ItemVariant, item_buffer_size> item_buffer_;
};
