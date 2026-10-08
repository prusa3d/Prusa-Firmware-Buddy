/// @file
#include <puppies/cyphal_node_table_host.hpp>

#include <bsod/bsod.h>

namespace buddy::puppies {

void CyphalNodeTableHost::invalidate() {
    last_sequence = std::nullopt;
    for (auto &node : nodes) {
        node.store({});
    }
}

CommunicationStatus CyphalNodeTableHost::refresh(PuppyModbus &bus, modbus::ServerAddress server, uint16_t &sequence) {
    if (last_sequence == sequence) {
        return CommunicationStatus::SKIPPED;
    }

    xbuddy_extension::modbus::CyphalNodeTable table;
    if (!bus.read_input_registers(server, table)) {
        return CommunicationStatus::ERROR;
    }

    for (size_t i = 0; i < nodes.size(); ++i) {
        nodes[i].store(xbuddy_extension::modbus::parse_cyphal_node_table_entry(table.nodes[i]));
    }

    sequence = table.sequence;
    last_sequence = table.sequence;
    return CommunicationStatus::OK;
}

xbuddy_extension::modbus::CyphalNodeTableEntry CyphalNodeTableHost::get_node(size_t index) const {
    debug_assert(index < nodes.size());
    return nodes[index].load();
}

xbuddy_extension::NodeState CyphalNodeTableHost::get_node_state(xbuddy_extension::NodeName name) const {
    using xbuddy_extension::NodeState;
    for (const auto &node : nodes) {
        if (const auto entry = node.load(); entry.name == name && entry.state != NodeState::unknown) {
            return entry.state;
        }
    }
    return NodeState::unknown;
}

} // namespace buddy::puppies
