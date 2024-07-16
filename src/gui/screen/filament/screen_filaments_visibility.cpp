#include "screen_filaments_visibility.hpp"

#include <ScreenHandler.hpp>
#include <algorithm_extensions.hpp>
#include <filament_list.hpp>
#include <filament_gui.hpp>

using namespace screen_filaments_visibility;

// * MI_FILAMENT
MI_FILAMENT::MI_FILAMENT(FilamentType filament_type)
    : WI_ICON_SWITCH_OFF_ON_t(false, {})
    , filament_type(filament_type)
    , filament_params(filament_type.parameters()) //
{
    FilamentTypeGUI::setup_menu_item(filament_type, filament_params, *this);
    set_value(filament_type.is_visible(), false);
}

void MI_FILAMENT::OnChange(size_t) {
    filament_type.set_visible(value());
    FilamentTypeGUI::setup_menu_item(filament_type, filament_params, *this);
}

// * WindowMenuFilamentsVisibility
WindowMenuFilamentsVisibility::WindowMenuFilamentsVisibility(window_t *parent, Rect16 rect)
    : WindowMenuVirtual(parent, rect, CloseScreenReturnBehavior::yes) //
{
    generate_filament_list(filament_list, management_generate_filament_list_config);
    setup_items();
}

int WindowMenuFilamentsVisibility::item_count() const {
    return filament_list.size() + 1;
}

void WindowMenuFilamentsVisibility::setup_item(ItemVariant &variant, int index) {
    if (index == 0) {
        variant.emplace<MI_RETURN>();
    } else {
        variant.emplace<MI_FILAMENT>(filament_list[index - 1]);
    }
}

// * ScreenFilamentManagementList
ScreenFilamentsVisibility::ScreenFilamentsVisibility()
    : ScreenMenuBase(nullptr, _("FILAMENTS VISIBILITY"), EFooter::Off) {
}
