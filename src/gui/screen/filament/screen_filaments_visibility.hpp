#pragma once

#include <window_menu_virtual.hpp>
#include <filament_list.hpp>

#include <MItem_tools.hpp>
#include <screen_menu.hpp>
#include <filament_list.hpp>

namespace screen_filaments_visibility {

class MI_FILAMENT final : public WI_ICON_SWITCH_OFF_ON_t {
public:
    MI_FILAMENT(FilamentType filament_type);

protected:
    void OnChange(size_t) final;

private:
    const FilamentType filament_type;
    const FilamentTypeParameters filament_params;
};

class WindowMenuFilamentsVisibility final : public WindowMenuVirtual<MI_RETURN, MI_FILAMENT> {

public:
    WindowMenuFilamentsVisibility(window_t *parent, Rect16 rect);

public:
    int item_count() const final;

protected:
    void setup_item(ItemVariant &variant, int index) final;

private:
    FilamentListStorage filament_list;
};

} // namespace screen_filaments_visibility

class ScreenFilamentsVisibility final : public ScreenMenuBase<screen_filaments_visibility::WindowMenuFilamentsVisibility> {
public:
    ScreenFilamentsVisibility();
};
