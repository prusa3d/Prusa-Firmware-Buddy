#include "screen_filament_management_list.hpp"
#include "screen_filament_detail.hpp"

#include <dynamic_index_mapping.hpp>
#include <ScreenHandler.hpp>
#include <algorithm_extensions.hpp>
#include <filament_list.hpp>
#include <filament_gui.hpp>
#include <encoded_filament.hpp>

using namespace screen_filament_management_list;

namespace {

enum class Item {
    return_,
    filament,
};

}

static constexpr auto items = std::to_array<DynamicIndexMappingRecord<Item>>({
    Item::return_,
    { Item::filament, DynamicIndexMappingType::static_section, total_filament_type_count },
});

static constexpr DynamicIndexMapping<items> index_mapping;

// * MI_FILAMENT
MI_FILAMENT::MI_FILAMENT(FilamentType filament_type)
    : IWindowMenuItem({}, nullptr, is_enabled_t::yes, is_hidden_t::no, expands_t::yes)
    , filament_type(filament_type)
    , filament_name(filament_type.parameters().name) //
{
    FilamentTypeGUI::setup_menu_item(filament_type, filament_name, *this);
}

void MI_FILAMENT::click(IWindowMenu &) {
    static constexpr std::array screen_factory = [] {
        std::array<ScreenFactory::Creator, total_filament_type_count> r;
        stdext::visit_sequence<total_filament_type_count>([&]<size_t i>() {
            // FilamentType is not "structural", so we cannot pass it as a template parameter -> pass EncodedFilamentType instead
            r[i] = ScreenFactory::Screen<ScreenFilamentDetail, EncodedFilamentType(all_filament_types[i])>;
        });
        return r;
    }();
    Screens::Access()->Open(screen_factory[stdext::index_of(all_filament_types, filament_type)]);
}

// * WindowMenuFilamentManagementList
WindowMenuFilamentManagementList::WindowMenuFilamentManagementList(window_t *parent, Rect16 rect)
    : WindowMenuVirtual(parent, rect, CloseScreenReturnBehavior::yes) //
{
    generate_filament_list(filament_list, management_generate_filament_list_config);
    setup_items();
}

int WindowMenuFilamentManagementList::item_count() const {
    return index_mapping.total_item_count();
}

void WindowMenuFilamentManagementList::setup_item(ItemVariant &variant, int index) {
    const auto mapping = index_mapping.from_index(index);
    switch (mapping.item) {

    case Item::return_:
        variant.emplace<MI_RETURN>();
        break;

    case Item::filament:
        variant.emplace<MI_FILAMENT>(filament_list[mapping.pos_in_section]);
        break;
    }
}

// * ScreenFilamentManagementList
ScreenFilamentManagementList::ScreenFilamentManagementList()
    : ScreenMenuBase(nullptr, _("FILAMENT MANAGEMENT"), EFooter::Off) {
}
