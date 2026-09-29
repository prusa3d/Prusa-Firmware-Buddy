/// @file
#include "screen_tool_pick_park.hpp"

#include <screen_menu.hpp>
#include <window_menu_virtual.hpp>
#include <dynamic_index_mapping.hpp>
#include <common/marlin_vars.hpp>
#include <utils/variant_utils.hpp>
#include <img_resources.hpp>
#include <string_builder.hpp>

namespace {

class MenuItemTool : public WiInfo<64> {

public:
    using Tool = std::variant<PhysicalToolIndex, NoTool>;

    MenuItemTool(Tool tool)
        : WiInfo<64>(string_view_utf8 {})
        , tool_(tool) {

        if (auto t = stdext::get_optional<PhysicalToolIndex>(tool)) {
            StringBuilder sb(value_array_);
            t->build_details(sb);
            value_ = string_view_utf8::MakeRAM(value_array_.data());
            update_extension_width();
        }
    }

    void click(IWindowMenu &) override {
        marlin_client::gcode("G27 P0 Z5"); // Lift Z if not high enough

        const bool is_tool = std::holds_alternative<PhysicalToolIndex>(tool_);
        if (is_tool && !is_selected_) {
            // Pick
            marlin_client::gcode_printf("T%d S1 L0 D0", std::get<PhysicalToolIndex>(tool_).to_raw());
            is_selecting_ = true;

        } else {
            // Park
            marlin_client::gcode_printf("P0 S1 L0");
            is_selecting_ = !is_tool;
        }
    }

    void Loop() override {
        const bool is_processing = marlin_vars().is_processing;

        if (!is_processing) {
            // Only update after everything is done to avoid ugly intermittent states
            is_selected_ = (PhysicalToolIndex::currently_selected() == tool_);
            is_selecting_ &= !is_selected_;
        }

        if (std::holds_alternative<PhysicalToolIndex>(tool_)) {
            const auto t = std::get<PhysicalToolIndex>(tool_);

            set_enabled(t.is_enabled() && !is_processing);

            const char *label = is_selected_ ? N_("Park Tool %i") : N_("Pick Tool %i");
            SetLabel(_(label).formatted(label_params_, t.display_index()));

        } else {
            set_enabled(!is_processing);
            SetLabel(_("Park Current Tool"));
        }

        const img::Resource *icon = nullptr;
        if (is_processing && (is_selected_ || is_selecting_)) {
            icon = img::spinner_16x16_animated();

        } else if (is_selected_) {
            icon = &img::arrow_right_10x16;
        }
        SetIconId(icon);
    }

private:
    const Tool tool_;
    bool is_selected_ = false;
    bool is_selecting_ = false;
    StringViewUtf8Parameters<4> label_params_;
};

enum class Item {
    return_,
    park,
    tool
};

static constexpr auto index_mapping_items = std::to_array<DynamicIndexMappingRecord<Item>>({
    Item::return_,
    Item::park,
    { Item::tool, DynamicIndexMappingType::dynamic_section },
});

class MenuToolPickPark : public WindowMenuVirtual {

public:
    MenuToolPickPark(window_frame_t *parent, Rect16 rect)
        : WindowMenuVirtual(parent, rect, CloseScreenReturnBehavior::yes) {

        index_mapping_.set_section_size<Item::tool>(PhysicalToolIndex::enabled_range_size());
        setup_items();
    }

    int item_count() const final {
        return index_mapping_.total_item_count();
    }

    void setup_item(ItemVariant &variant, int index) final {
        const auto m = index_mapping_.from_index(index);

        switch (m.item) {

        case Item::return_:
            variant.emplace<MI_RETURN>();
            break;

        case Item::park:
            variant.emplace<MenuItemTool>(NoTool {});
            break;

        case Item::tool:
            variant.emplace<MenuItemTool>(PhysicalToolIndex::from_raw(m.pos_in_section));
            break;
        }
    }

private:
    DynamicIndexMapping<index_mapping_items> index_mapping_;
};

class ScreenToolPickPark : public ScreenMenuBase<MenuToolPickPark> {

public:
    ScreenToolPickPark()
        : ScreenMenuBase(nullptr, _("TOOL SELECT"), EFooter::Off) {}
};

} // namespace

ScreenFactory::Creator
screen_tool_pick_park_creator() {
    return ScreenFactory::Screen<ScreenToolPickPark>;
}
