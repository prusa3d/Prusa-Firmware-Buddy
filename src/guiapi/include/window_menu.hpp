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
    uint8_t index_of_first;             /// container index of first item on screen
    uint8_t max_items_on_screen;
    uint8_t visible_count_at_last_draw; // to redraw last item, if it was hidden, has no effect in case entire window is invalid
    IWinMenuContainer *pContainer;

    std::optional<Rect16> getItemRC(size_t position_on_screen) const;
    void setIndex(uint8_t index); // for ctor (cannot fail)
    /// Prints single item in the menu
    void printItem(IWindowMenuItem &item, Rect16 rc);
    /// Searches for next visible item
    /// Repeats search for \param steps times
    /// Negative value searches in backward direction
    /// \returns false if end of item list reached before all steps consumed
    bool moveToNextVisibleItem(int moveIndex);
    /// Moves menu so the cursor is on the screen
    /// \returns true if menu was moved
    bool updateTopIndex_IsRedrawNeeded();
    /// Moves menu so the cursor is on the screen and invalidates if moved
    /// \returns true if menu was moved
    bool updateTopIndex();
    /// Plays proper sound according to item/value changed
    /// \returns input
    bool playEncoderSound(bool changed);

    using roll_fn = std::optional<uint8_t> (WindowMenu::*)() const;
    std::optional<uint8_t> calc_up_index() const;
    std::optional<uint8_t> calc_down_index() const;

    bool roll(roll_fn fn);

    std::optional<size_t> slotFromCoord(point_ui16_t point);
    IWindowMenuItem *itemFromSlot(size_t slot);

    struct Node {
        IWindowMenuItem *item;
        size_t current_slot;
        size_t index;

        bool HasValue() { return item; }
        static constexpr Node Empty() { return { nullptr, 0, 0 }; }
    };

    Node findFirst();
    Node findNext(Node prev);

public:
    static Rect16::Height_t ItemHeight();

    WindowMenu(window_t *parent, Rect16 rect, IWinMenuContainer *pContainer, uint8_t index = 0);
    void BindContainer(IWinMenuContainer &cont, uint8_t index = 0);

    bool SetIndex(uint8_t index); // must check container
    void Increment(int dif);
    void Decrement(int dif) { Increment(-dif); }
    std::optional<size_t> GetIndex() const;
    /// \returns visible index of item
    std::optional<size_t> GetIndex(IWindowMenuItem &item) const;
    /// \returns number of all menu items including hidden ones
    uint8_t GetCount() const;                      // count of all visible items in container
    IWindowMenuItem *GetItem(uint8_t index) const; // nth visible item in container
    IWindowMenuItem *GetActiveItem() const;        // focused item
    bool SetActiveItem(IWindowMenuItem &item) {
        std::optional<size_t> index = GetIndex(item);
        if (!index)
            return false;

        return SetIndex(*index);
    }

    void InitState(screen_init_variant::menu_t var);
    screen_init_variant::menu_t GetCurrentState() const;

    void Show(IWindowMenuItem &item);
    bool Hide(IWindowMenuItem &item);
    bool SwapVisibility(IWindowMenuItem &item0, IWindowMenuItem &item1);

    uint8_t GetMaxItemsOnScreen() const { return max_items_on_screen; }
    uint8_t GetIndexOfFirst() const { return index_of_first; }

    void RollUp();
    void RollDown();

protected:
    virtual void draw() override;
    virtual void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) override;

    // TODO
    // virtual void invalidate(Rect16 validation_rect);
    // virtual void validate(Rect16 validation_rect);
};
