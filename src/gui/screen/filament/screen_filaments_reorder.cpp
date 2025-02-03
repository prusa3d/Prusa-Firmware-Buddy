#include "screen_filaments_reorder.hpp"

#include <algorithm>
#include <dynamic_index_mapping.hpp>

#include <filament_gui.hpp>
#include <gui/event/focus_event.hpp>

using namespace screen_filaments_reorder;

namespace {

enum class Item {
    return_,
    filament_section,
    reset_to_defaults,
};

}

static constexpr auto index_mapping_items = std::to_array<DynamicIndexMappingRecord<Item>>({
    Item::return_,
    { Item::filament_section, DynamicIndexMappingType::static_section, total_filament_type_count },
    Item::reset_to_defaults,
});

static constexpr DynamicIndexMapping<index_mapping_items> index_mapping;

static constexpr IWindowMenuItem::ColorScheme moved_filament_color_scheme {
    .text = {
        .focused = COLOR_ORANGE,
        .unfocused = COLOR_ORANGE,
    },
};

// * MI_FILAMENT
MI_FILAMENT::MI_FILAMENT(WindowMenuFilamentsReorder &menu, size_t index)
    : IWindowMenuItem({})
    , menu(menu)
    , index(index) //
{
    update();
}

void MI_FILAMENT::update() {
    FilamentType new_filament_type = menu.filament_list[index];

    if (menu.moved_filament && menu.focused_item_index) {
        if (index == menu.focused_item_index) {
            // We're focused -> show currently moved item
            new_filament_type = *menu.moved_filament;

        } else if (index > menu.focused_item_index) {
            // We're behind focus - show "previous" filament, the list has been shifted because of the insertion
            new_filament_type = menu.filament_list[index - 1];
        }
    }

    if (filament_type != new_filament_type) {
        filament_type = new_filament_type;
        filament_name = filament_type.parameters().name;

        // Setting text does not invalidate, because it is the same reference -> do it manually
        Invalidate();
    }

    // Setup the menu item anyway, to make sure we've cleared the focused overrides
    FilamentTypeGUI::setup_menu_item(filament_type, filament_name, *this);

    // Override looks if we're focused
    if (menu.moved_filament && is_focused()) {
        set_color_scheme(&moved_filament_color_scheme);
        SetIconId(&img::move_16x16);
    }
}

void MI_FILAMENT::click(IWindowMenu &) {
    auto &list = menu.filament_list;

    // We were moving an item -> apply the movement
    if (menu.moved_filament) {
        // When we're moving a filament, we probably want to make it visible -> do so
        menu.moved_filament->set_visible(true);

        // Insert the item at the new location
        std::shift_right(list.begin() + index, list.end(), 1);
        list[index] = *menu.moved_filament;

        // Clear the selection
        menu.moved_filament = std::nullopt;

        // Save the ordering changes only on exiting to reduce config store writes
        menu.changed |= (index != menu.original_moved_filament_index);

    } else {
        menu.moved_filament = list[index];
        menu.original_moved_filament_index = index;

        // Remove the item from the original location
        std::shift_left(list.begin() + index, list.end(), 1);
    }

    menu.update_all();
}

void MI_FILAMENT::event(WindowMenuItemEventContext &ctx) {
    if (ctx.event.holds_alternative<gui_event::FocusInEvent>()) {
        menu.focused_item_index = index;

    } else if (ctx.event.holds_alternative<gui_event::FocusOutEvent>()) {
        menu.focused_item_index = std::nullopt;
    }

    // If we are moving an item, update all items on focus change - because their texts and properties change as the item is moved around
    if (menu.moved_filament) {
        menu.update_all();
    }

    IWindowMenuItem::event(ctx);
}

// * WindowMenuFilamentsReorder
WindowMenuFilamentsReorder::WindowMenuFilamentsReorder(window_t *parent, Rect16 rect)
    : WindowMenuVirtual(parent, rect, CloseScreenReturnBehavior::yes) //
{
    load_list();
}

WindowMenuFilamentsReorder::~WindowMenuFilamentsReorder() {
    // Save the changes only on exiting to reduce config store writes
    if (changed) {
        // The config store array is bigger than ours, we gotta memcpy
        decltype(config_store_ns::CurrentStore::filament_order)::value_type data;
        std::copy(filament_list.begin(), filament_list.end(), data.begin());
        config_store().filament_order.set(data);
    }
}

int WindowMenuFilamentsReorder::item_count() const {
    return index_mapping.total_item_count() - moved_filament.has_value();
}

void WindowMenuFilamentsReorder::setup_item(ItemVariant &variant, int index) {
    const auto mapping = index_mapping.from_index(index);
    switch (mapping.item) {

    case Item::return_:
        variant.emplace<MI_RETURN>();
        break;

    case Item::reset_to_defaults: {
        const auto callback = [this] {
            if (MsgBoxQuestion(_("Do you really want to restore default filament ordering?"), Responses_YesNo) != Response::Yes) {
                return;
            }

            config_store().filament_order.set_to_default();
            load_list();
            move_focus_to_index(std::nullopt);
            set_scroll_offset(0);
        };
        variant.emplace<WindowMenuCallbackItem>(_("Restore Defaults"), callback);
        break;
    }

    case Item::filament_section:
        variant.emplace<MI_FILAMENT>(*this, mapping.pos_in_section);
        break;
    }
}

void WindowMenuFilamentsReorder::update_all() {
    for (auto &i : buffered_items()) {
        if (std::holds_alternative<MI_FILAMENT>(i)) {
            std::get<MI_FILAMENT>(i).update();
        }
    }
}

void WindowMenuFilamentsReorder::load_list() {
    generate_filament_list(filament_list, management_generate_filament_list_config);
    setup_items();
}

// * ScreenFilamentsReorder
ScreenFilamentsReorder::ScreenFilamentsReorder()
    : ScreenMenuBase(nullptr, _("REORDER FILAMENTS"), EFooter::Off) {}
