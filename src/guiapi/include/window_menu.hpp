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
#include <window_menu_adv.hpp>

#include <cstdint>
#include <optional>

// todo
// use template instead IWinMenuContainer *pContainer;
// I want same methods for IWinMenuContainer as std::array<IWindowMenuItem *, N>  .. need to add iterators
class WindowMenu : public IWindowMenu {

private:
    IWinMenuContainer *pContainer = nullptr;

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
    WindowMenu(window_t *parent, Rect16 rect, IWinMenuContainer *pContainer = nullptr);
    void BindContainer(IWinMenuContainer &cont);

    std::optional<int> item_index_to_persistent_index(std::optional<int> item_index) const override;
    std::optional<int> persistent_index_to_item_index(std::optional<int> persistent_index) const override;

    /// \returns visible index of item
    std::optional<int> GetIndex(IWindowMenuItem &item) const;

    int item_count() const final;

    IWindowMenuItem *item_at(int index) final;

    std::optional<int> item_index(const IWindowMenuItem *item) const final;

    bool SwapVisibility(IWindowMenuItem &item0, IWindowMenuItem &item1);

protected:
    virtual void windowEvent(window_t *sender, GUI_event_t event, void *param) override;

    // TODO
    // virtual void invalidate(Rect16 validation_rect);
    // virtual void validate(Rect16 validation_rect);
};

using window_menu_t = WindowExtendedMenu<WindowMenu>;
