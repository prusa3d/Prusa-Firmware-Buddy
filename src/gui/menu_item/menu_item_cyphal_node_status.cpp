/// @file
#include "menu_item_cyphal_node_status.hpp"

#include <i18n.h>
#include <puppies/cyphal_bridge_host.hpp>
#include <puppies/cyphal_node_table_host.hpp>
#include <string_builder.hpp>
#include <utils/enum_array.hpp>

using xbuddy_extension::NodeName;
using xbuddy_extension::NodeState;

MenuItemCyphalNodeStatus::MenuItemCyphalNodeStatus(const string_view_utf8 &label, NodeName node_name)
    : MenuItemAutoUpdatingLabel(
        label,
        [this](const std::span<char> &buffer) { print_val(buffer); },
        [](MenuItemAutoUpdatingLabel<CyphalNodeStatus> *item) { return static_cast<MenuItemCyphalNodeStatus *>(item)->get_status(); })
    , node_name_(node_name) {}

CyphalNodeStatus MenuItemCyphalNodeStatus::get_status() const {
    const auto state = buddy::puppies::cyphal_node_table().get_node_state(node_name_);
    const auto &bridge = buddy::puppies::cyphal_bridge_host();
    if (state == NodeState::flash && bridge.get_flash_file_id() == xbuddy_extension::firmware_file_id(node_name_)) {
        return { .state = state, .percent = bridge.get_flash_progress_percent() };
    }
    return { .state = state, .percent = std::nullopt };
}

void MenuItemCyphalNodeStatus::print_val(const std::span<char> &buffer) const {
    static constexpr EnumArray<NodeState, const char *, 4> texts {
        { NodeState::unknown, N_("Not detected") },
        { NodeState::verify, N_("Verifying") },
        { NodeState::flash, N_("Flashing") },
        { NodeState::ready, N_("Ready") },
    };
    StringBuilder sb(buffer);
    sb.append_string_view(_(texts[value().state]));
    if (value().percent) {
        sb.append_printf(" %u %%", *value().percent);
    }
}
