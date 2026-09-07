#include "screen_opt_tag_list.hpp"
#include "screen_opt_info.hpp"

#include <inplace_vector.hpp>

#include <WindowMenuItems.hpp>
#include <ScreenHandler.hpp>
#include <screen_menu.hpp>
#include <window_menu_virtual.hpp>
#include <dynamic_index_mapping.hpp>
#include <gui/screen/screen_menu_virtual.hpp>

#include <feature/openprinttag/tool_tag.hpp>
#include <bsod/bsod.h>

namespace buddy::openprinttag {

#if EXTRUDERS > 1
namespace {
    static constexpr screen_menu_virtual::Configuration menu_config {
        .item_count = [] -> int {
            return VirtualToolIndex::enabled_range_size() + 1; // + MI_RETURN
        },
        .item_constructor = [](WindowMenuVirtual::ItemVariant &variant, int index) {
            if (index == 0) {
                variant.emplace<MI_RETURN>();
            } else {
                variant.emplace<MI_OPT_READ_TAG>(VirtualToolIndex::from_raw(index - 1));
            }
            //
        },
        .title = N_("READ OPENPRINTTAG"),
    };
} // namespace
#endif

MI_OPT_READ_TAG::MI_OPT_READ_TAG(Tool tool)
    : tool_(tool) {
    set_show_expand_icon();

    // !!! Needs to be BEFORE collapsing to a single tool
    if (auto *tool = std::get_if<VirtualToolIndex>(&tool_)) {
        SetLabel(tool->display_name(label_params_));
    } else {
        SetLabel(_("Read OpenPrintTag"));
    }

    // Collapse to a single tool if the printer doesn't have more tools
    if (auto single_tool = VirtualToolIndex::single_enabled_tool(); std::holds_alternative<AllTools>(tool_) && single_tool.has_value()) {
        tool_ = *single_tool;
    }
}

void MI_OPT_READ_TAG::Loop() {
    if (auto *tool = std::get_if<VirtualToolIndex>(&tool_)) {
        // Indicate the item as disabled, but do not really disable it
        // This is so that the click() function gets called even if the item is disabled
        set_color_scheme(ToolTag::for_tool_ephemeral(*tool).has_value() ? &color_scheme_default : &color_scheme_default_disabled);
    }
}

void MI_OPT_READ_TAG::click(IWindowMenu &) {
    if (auto *tool = std::get_if<VirtualToolIndex>(&tool_)) {
        // Open the screen even if the slot is "disabled"
        // The screen will show an error and close if there is no tag detected
        Screens::Access()->Open(screen_opt_info_ephemeral_creator(*tool));

    } else {
#if EXTRUDERS > 1
        Screens::Access()->Open(ScreenFactory::ScreenWithArg<ScreenMenuVirtual>(&menu_config));
#else
        bsod_unreachable();
#endif
    }
}

} // namespace buddy::openprinttag
