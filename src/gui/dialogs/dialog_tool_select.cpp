/// @file
#include "dialog_tool_select.hpp"

#include <algorithm>

#include <window_menu_adv.hpp>
#include <window_menu_virtual.hpp>
#include <window_menu_callback_item.hpp>
#include <WindowMenuItems.hpp>
#include <utils/variant_utils.hpp>
#include <img_resources.hpp>
#include <ScreenHandler.hpp>
#include <window_header.hpp>
#include <dynamic_index_mapping.hpp>
#include <string_builder.hpp>
#include <config_store/store_instance.hpp>

namespace {

enum class Item {
    return_,
    prefix_section,
    tool
};

static constexpr auto index_mapping_items = std::to_array<DynamicIndexMappingRecord<Item>>({
    { Item::return_, DynamicIndexMappingType::optional_item },
    { Item::prefix_section, DynamicIndexMappingType::dynamic_section },
    { Item::tool, DynamicIndexMappingType::dynamic_section },
});

class SelectToolMenu;

class MI_TOOL final : public WiInfo<64> {

public:
    MI_TOOL(SelectToolMenu &menu, VirtualToolIndex tool)
        : WiInfo(string_view_utf8 {})
        , menu_(menu)
        , tool_(tool) {

        SetLabel(tool.display_name(label_params_));

        if (stdext::holds_value(VirtualToolIndex::currently_selected(), tool)) {
            SetIconId(&img::arrow_right_10x16);
        }

        StringBuilder sb(value_array_);
        tool_.build_details(sb);
        value_ = string_view_utf8::MakeRAM(value_array_.data());
        update_extension_width();
    }

    void click(IWindowMenu &) override;

private:
    SelectToolMenu &menu_;
    const VirtualToolIndex tool_;
    StringViewUtf8Parameters<8> label_params_;
};

class SelectToolMenu : public WindowMenuVirtual {
    friend class MI_TOOL;

public:
    SelectToolMenu(window_frame_t *parent, Rect16 rect, const SelectToolDialogArgs &args)
        : WindowMenuVirtual(parent, rect, args.allow_return ? CloseScreenReturnBehavior::yes : CloseScreenReturnBehavior::no)
        , args_(args) {

        index_mapping_.set_item_enabled<Item::return_>(args.allow_return);
        index_mapping_.set_section_size<Item::tool>(VirtualToolIndex::enabled_range_size());
        index_mapping_.set_section_size<Item::prefix_section>(args.prefix_section_size);

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

        case Item::prefix_section:
            args_.prefix_section_ctor(variant, m.pos_in_section);
            break;

        case Item::tool: {
            const auto tool = VirtualToolIndex::from_raw(m.pos_in_section);
            MI_TOOL &item = variant.emplace<MI_TOOL>(*this, tool);
            item.set_enabled(tool.is_enabled() && args_.tool_filter(tool));
            break;
        }
        }
    }

public:
    std::optional<VirtualToolIndex> result;

private:
    const SelectToolDialogArgs args_;
    DynamicIndexMapping<index_mapping_items> index_mapping_;
};

void MI_TOOL::click(IWindowMenu &) {
    menu_.result = tool_;
    Screens::Access()->close_dialog();
}

class SelectToolDialog final : public IDialog {

public:
    SelectToolDialog(const SelectToolDialogArgs &args)
        : header_(this)
        , menu_(this, GuiDefaults::RectScreenNoHeader, args) {
        CaptureNormalWindow(menu_);
    }

    auto result() const {
        return menu_.menu.result;
    }

private:
    window_header_t header_;
    WindowExtendedMenu<SelectToolMenu> menu_;
};
} // namespace

std::optional<VirtualToolIndex> select_tool_dialog(const SelectToolDialogArgs &args) {
    // No point selecting if there is only one tool enabled
    if (auto t = VirtualToolIndex::single_enabled_tool(); t.has_value() && args.tool_filter(*t)) {
        return *t;
    }

    SelectToolDialog d(args);
    Screens::Access()->gui_loop_until_dialog_closed();
    return d.result();
}
