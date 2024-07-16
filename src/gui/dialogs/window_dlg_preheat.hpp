#pragma once

#include <filament.hpp>
#include <string_view_utf8.hpp>
#include <screen_menu.hpp>
#include <IDialogMarlin.hpp>
#include <filament_list.hpp>
#include <dynamic_index_mapping.hpp>
#include <window_menu_virtual.hpp>

#include <MItem_tools.hpp>
#include <fsm_preheat_type.hpp>

namespace preheat_menu {

class WindowMenuPreheat;

// extra space at the end is intended
class MI_FILAMENT : public WiInfo<sizeof("999/999 ")> {
public:
    MI_FILAMENT(FilamentType filament_type);
    void click(IWindowMenu &) final;

    const FilamentType filament_type;
    const FilamentTypeParameters filament_params;
};

class MI_RETURN_PREHEAT : public IWindowMenuItem {
public:
    MI_RETURN_PREHEAT();
    virtual void click(IWindowMenu &window_menu);
};

class MI_SHOW_ALL : public IWindowMenuItem {
public:
    MI_SHOW_ALL(WindowMenuPreheat &menu);
    virtual void click(IWindowMenu &);

private:
    WindowMenuPreheat &menu;
};

class MI_COOLDOWN : public IWindowMenuItem {
public:
    MI_COOLDOWN();
    virtual void click(IWindowMenu &window_menu);
};

class WindowMenuPreheat : public WindowMenuVirtual<MI_FILAMENT, MI_SHOW_ALL, MI_RETURN_PREHEAT, MI_COOLDOWN> {

public:
    WindowMenuPreheat(window_t *parent, const Rect16 &rect, const PreheatData &data);

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
    };

    static constexpr auto items = std::to_array<DynamicIndexMappingRecord<Item>>({
        { Item::return_, DynamicIndexMappingType::optional_item },
        { Item::filament_section, DynamicIndexMappingType::dynamic_section },
        { Item::cooldown, DynamicIndexMappingType::optional_item },
        { Item::show_all, DynamicIndexMappingType::optional_item },
    });

private:
    FilamentListStorage filament_list_storage;
    DynamicIndexMapping<items> index_mapping;
    bool show_all_filaments_ = false;
};

class DialogMenuPreheat : public IDialogMarlin {
    WindowExtendedMenu<WindowMenuPreheat> menu;
    window_header_t header;

public:
    DialogMenuPreheat(fsm::BaseData data);

protected:
    static string_view_utf8 get_title(fsm::BaseData data);
};

}; // namespace preheat_menu

using DialogMenuPreheat = preheat_menu::DialogMenuPreheat;
