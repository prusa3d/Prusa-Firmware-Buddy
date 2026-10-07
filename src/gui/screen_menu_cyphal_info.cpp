/// @file
#include "screen_menu_cyphal_info.hpp"

#include <bsod/bsod.h>
#include <gui/img_resources.hpp>
#include <gui/screen/screen_menu_virtual.hpp>
#include <puppies/cyphal_node_table_host.hpp>
#include <ScreenFactory.hpp>
#include <ScreenHandler.hpp>

namespace {

using xbuddy_extension::NodeName;
using xbuddy_extension::NodeState;
using xbuddy_extension::modbus::CyphalNodeTableEntry;

const char *to_string(NodeName name) {
    switch (name) {
    case NodeName::none:
        return "none";
    case NodeName::cz_prusa3d_honeybee_ac_controller:
        return "cz.prusa3d.honeybee.ac_controller";
    case NodeName::cz_prusa3d_honeybee_nfc:
        return "cz.prusa3d.honeybee.nfc";
    case NodeName::cz_prusa3d_honeybee_tool_offset_sensor:
        return "cz.prusa3d.honeybee.tool_offset_sensor";
    }
    bsod_unreachable();
}

const char *to_string(NodeState state) {
    switch (state) {
    case NodeState::unknown:
        return "unknown";
    case NodeState::verify:
        return "verify";
    case NodeState::flash:
        return "flash";
    case NodeState::ready:
        return "ready";
    }
    bsod_unreachable();
}

CyphalNodeTableEntry get_node(uint8_t node_id) {
    return buddy::puppies::cyphal_node_table().get_node(node_id);
}

string_view_utf8 node_name(uint8_t node_id) {
    return string_view_utf8::MakeCPUFLASH(to_string(get_node(node_id).name));
}

bool is_present(uint8_t node_id) {
    return get_node(node_id) != CyphalNodeTableEntry {};
}

int node_count() {
    int count = 0;
    for (uint8_t node_id = 0; node_id < xbuddy_extension::cyphal_node_count; ++node_id) {
        count += is_present(node_id);
    }
    return count;
}

uint8_t nth_node(int n) {
    for (uint8_t node_id = 0; node_id < xbuddy_extension::cyphal_node_count; ++node_id) {
        if (is_present(node_id) && n-- == 0) {
            return node_id;
        }
    }
    return 0;
}

constexpr screen_menu_virtual::Configuration cyphal_info_configuration {
    .item_count = [] -> int { return node_count() + 1; }, // + MI_RETURN
    .item_constructor = [](WindowMenuVirtual::ItemVariant &variant, int index) {
        if (index == 0) {
            variant.emplace<MI_RETURN>();
        } else {
            variant.emplace<MI_CYPHAL_NODE>(nth_node(index - 1));
        } },
    .title = N_("CYPHAL INFO"),
};

} // namespace

MI_CYPHAL_INFO::MI_CYPHAL_INFO()
    : MI_SCREEN_BASE {
        ScreenFactory::ScreenWithArg<ScreenMenuVirtual>(&cyphal_info_configuration),
        N_("Cyphal Info"),
    } {}

MI_CYPHAL_NODE::MI_CYPHAL_NODE(uint8_t node_id)
    : IWindowMenuItem {
        node_name(node_id),
        nullptr,
        is_enabled_t::yes,
        is_hidden_t::no,
        expands_t::yes,
    }
    , node_id { node_id } {}

void MI_CYPHAL_NODE::click(IWindowMenu &) {
    Screens::Access()->Open(ScreenFactory::ScreenWithArg<ScreenMenuCyphalNode, uint8_t>(node_id));
}

MI_CYPHAL_NODE_ID::MI_CYPHAL_NODE_ID()
    : WI_FORMATABLE_LABEL_t<uint8_t> {
        _("Node ID"),
        "%u",
        0,
    } {}

MI_CYPHAL_NODE_STATE::MI_CYPHAL_NODE_STATE()
    : WiInfoString {
        {},
        _("State")
    } {}

void MI_CYPHAL_NODE_STATE::set_node_id(uint8_t new_node_id) {
    node_id = new_node_id;
    Loop();
}

void MI_CYPHAL_NODE_STATE::Loop() {
    set_value(string_view_utf8::MakeCPUFLASH(to_string(get_node(node_id).state)));
}

ScreenMenuCyphalNode::ScreenMenuCyphalNode(uint8_t node_id)
    : ScreenMenuCyphalNodeBase {
        node_name(node_id),
        &img::info_16x16,
    } {
    Item<MI_CYPHAL_NODE_ID>().UpdateValue(node_id);
    Item<MI_CYPHAL_NODE_STATE>().set_node_id(node_id);
}
