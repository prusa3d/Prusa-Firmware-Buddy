#pragma once

#include <filament.hpp>
#include <string_view_utf8.hpp>
#include <screen_menu.hpp>
#include <screen_fsm.hpp>
#include <filament_list.hpp>
#include <dynamic_index_mapping.hpp>
#include <window_menu_virtual.hpp>
#include <window_menu_callback_item.hpp>

#include <MItem_tools.hpp>
#include <fsm_preheat_type.hpp>

namespace preheat_menu {

class WindowMenuPreheat;

// extra space at the end is intended
class MI_FILAMENT : public WiInfo<sizeof("999/999 ")> {
public:
    MI_FILAMENT(FilamentType filament_type, uint8_t target_extruder);
    void click(IWindowMenu &) final;

    const FilamentType filament_type;
    const FilamentTypeParameters filament_params;
    const uint8_t target_extruder;
};

class WindowMenuPreheat : public WindowMenuVirtual<WindowMenuCallbackItem, MI_FILAMENT> {

public:
    WindowMenuPreheat(window_t *parent, const Rect16 &rect);

    void set_data(const PreheatData &data);
    void set_show_all_filaments(bool set);

    int item_count() const final {
        return index_mapping.total_item_count();
    }

protected:
    void update_list();
    void setup_item(ItemVariant &variant, int index) final;

protected:
    void screenEvent(window_t *sender, GUI_event_t event, void *param) override;

private:
    enum class Item {
        return_,
        filament_section,
        show_all,
        cooldown,
        adhoc_filament,
    };

    static constexpr auto items = std::to_array<DynamicIndexMappingRecord<Item>>({
        { Item::return_, DynamicIndexMappingType::optional_item },
        { Item::filament_section, DynamicIndexMappingType::dynamic_section },
        { Item::adhoc_filament },
        { Item::cooldown, DynamicIndexMappingType::optional_item },
        { Item::show_all, DynamicIndexMappingType::optional_item },
    });

private:
    PreheatData preheat_data;
    FilamentListStorage filament_list_storage;
    DynamicIndexMapping<items> index_mapping;
    bool show_all_filaments_ = false;

    /// Extruder we're doing the preload for
    uint8_t extruder_index = 0;
};

class ScreenPreheat : public ScreenFSM {
    WindowExtendedMenu<WindowMenuPreheat> menu;
    window_header_t header;

public:
    ScreenPreheat();

protected:
    void create_frame();
    void destroy_frame();
    void update_frame();
};

}; // namespace preheat_menu

using ScreenPreheat = preheat_menu::ScreenPreheat;
