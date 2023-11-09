/**
 * @file window_menu.hpp
 * @brief simple window representing menu, without additional widgets like scrollbar
 */

#pragma once

#include "i_window_menu.hpp"
#include "i_window_menu_container.hpp"
#include "screen_init_variant.hpp"
#include "window_icon.hpp"
#include "window_frame.hpp"

#include <cstdint>
#include <optional>

// todo
// use template instead IWinMenuContainer *pContainer;
// I want same methods for IWinMenuContainer as std::array<IWindowMenuItem *, N>  .. need to add iterators
class WindowMenu : public AddSuperWindow<IWindowMenu> {

private:
    uint8_t visible_count_at_last_draw; // to redraw last item, if it was hidden, has no effect in case entire window is invalid
    IWinMenuContainer *pContainer;

    /// Prints single item in the menu
    void printItem(IWindowMenuItem &item, Rect16 rc);

    /// Plays proper sound according to item/value changed
    /// \returns input
    bool playEncoderSound(bool changed);

    void set_scroll_offset(int set) final;

    IWindowMenuItem *itemFromSlot(int slot);

    struct Node {
        IWindowMenuItem *item;
        int current_slot;
        int index;

        bool HasValue() { return item; }
        static constexpr Node Empty() { return { nullptr, 0, 0 }; }
    };

    Node findFirst();
    Node findNext(Node prev);

public:
    WindowMenu(window_t *parent, Rect16 rect, IWinMenuContainer *pContainer);
    void BindContainer(IWinMenuContainer &cont);

    /// Ugly legacy function REMOVEME
    inline void Increment(int amount) {
        move_focus_by(amount, YNPlaySound::yes);
    }

    /// Ugly legacy function REMOVEME
    inline void Decrement(int amount) {
        move_focus_by(-amount, YNPlaySound::yes);
    }

    std::optional<int> focused_item_index() const final;

    bool move_focus_to_index(std::optional<int> index) final;

    /// \returns visible index of item
    std::optional<int> GetIndex(IWindowMenuItem &item) const;

    int item_count() const final;

    IWindowMenuItem *GetItem(int index) const; // nth visible item in container

    bool SetActiveItem(IWindowMenuItem &item) {
        const auto index = GetIndex(item);
        if (!index) {
            return false;
        }

        return move_focus_to_index(*index);
    }

    void InitState(screen_init_variant::menu_t var);
    screen_init_variant::menu_t GetCurrentState() const;

    void Show(IWindowMenuItem &item);
    bool Hide(IWindowMenuItem &item);
    bool SwapVisibility(IWindowMenuItem &item0, IWindowMenuItem &item1);

protected:
    virtual void draw() override;
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;

    // TODO
    // virtual void invalidate(Rect16 validation_rect);
    // virtual void validate(Rect16 validation_rect);
};
