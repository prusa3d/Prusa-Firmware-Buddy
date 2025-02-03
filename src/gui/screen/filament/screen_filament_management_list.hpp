#pragma once

#include <window_menu_virtual.hpp>
#include <filament_list.hpp>
#include <screen_menu.hpp>

#include <MItem_tools.hpp>

namespace screen_filament_management_list {

class MI_FILAMENT final : public IWindowMenuItem {
public:
    MI_FILAMENT(FilamentType filament_type);

protected:
    void click(IWindowMenu &);

private:
    const FilamentType filament_type;
    const FilamentTypeParameters::Name filament_name;
};

class WindowMenuFilamentManagementList final : public WindowMenuVirtual<MI_RETURN, MI_FILAMENT> {
public:
    WindowMenuFilamentManagementList(window_t *parent, Rect16 rect);

public:
    int item_count() const final;

protected:
    void setup_item(ItemVariant &variant, int index) final;

private:
    FilamentListStorage filament_list;
};

} // namespace screen_filament_management_list

class ScreenFilamentManagementList final : public ScreenMenuBase<screen_filament_management_list::WindowMenuFilamentManagementList> {
public:
    ScreenFilamentManagementList();
};
