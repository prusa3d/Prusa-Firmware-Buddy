#pragma once

#include <window_menu_virtual.hpp>
#include <filament_list.hpp>

#include <MItem_tools.hpp>
#include <screen_menu.hpp>
#include <window_menu_callback_item.hpp>

namespace screen_filaments_reorder {

class WindowMenuFilamentsReorder;

class MI_FILAMENT final : public IWindowMenuItem {
public:
    MI_FILAMENT(WindowMenuFilamentsReorder &menu, size_t index);
    void update();

protected:
    void click(IWindowMenu &) override;
    void event(WindowMenuItemEventContext &ctx) override;

private:
    WindowMenuFilamentsReorder &menu;
    const size_t index;
    FilamentType filament_type;
    FilamentTypeParameters filament_params;
};

class WindowMenuFilamentsReorder final : public WindowMenuVirtual<WindowMenuCallbackItem, MI_RETURN, MI_FILAMENT> {
    friend class MI_FILAMENT;

public:
    WindowMenuFilamentsReorder(window_t *parent, Rect16 rect);
    ~WindowMenuFilamentsReorder();

public:
    int item_count() const final;

protected:
    void setup_item(ItemVariant &variant, int index) final;

    void update_all();

private:
    void load_list();

private:
    FilamentListStorage filament_list;

    /// Filament type that is currently moved
    std::optional<FilamentType> moved_filament;

    std::optional<size_t> original_moved_filament_index;

    /// Index of the MI_FILAMENT that is currently being focused
    std::optional<size_t> focused_item_index;

    /// Stores whether the ordering was changed -> the menu should save the changes when exiting
    bool changed = false;
};

} // namespace screen_filaments_reorder

class ScreenFilamentsReorder final : public ScreenMenuBase<screen_filaments_reorder::WindowMenuFilamentsReorder> {
public:
    ScreenFilamentsReorder();
};
