/// @file
#include <xbuddy_extension/modbus.hpp>

#include <modbus/traits.hpp>

static_assert(modbus::RegisterFile<xbuddy_extension::modbus::Status>);
static_assert(modbus::RegisterFile<xbuddy_extension::modbus::Config>);
static_assert(modbus::RegisterFile<xbuddy_extension::modbus::Chunk>);
static_assert(modbus::RegisterFile<xbuddy_extension::modbus::Digest>);
static_assert(modbus::RegisterFile<xbuddy_extension::modbus::LogMessage>);
static_assert(modbus::RegisterFile<xbuddy_extension::modbus::CyphalBridge>);
static_assert(modbus::RegisterFile<xbuddy_extension::modbus::CyphalNodeTable>);

// Chunk structure is optimized to transfer as much data as possible
// in a single MODBUS transaction to improve throughput.
static_assert(sizeof(xbuddy_extension::modbus::Chunk) == modbus::max_register_file_size_bytes);

namespace {

xbuddy_extension::NodeName parse_node_name(uint16_t name) {
    using xbuddy_extension::NodeName;
    switch (const auto value = static_cast<NodeName>(name)) {
    case NodeName::none:
    case NodeName::cz_prusa3d_honeybee_ac_controller:
    case NodeName::cz_prusa3d_honeybee_nfc:
    case NodeName::cz_prusa3d_honeybee_tool_offset_sensor:
        return value;
    }
    return NodeName::none;
}

xbuddy_extension::NodeState parse_node_state(uint16_t state) {
    using xbuddy_extension::NodeState;
    switch (const auto value = static_cast<NodeState>(state)) {
    case NodeState::unknown:
    case NodeState::verify:
    case NodeState::flash:
    case NodeState::ready:
        return value;
    }
    return NodeState::unknown;
}

} // namespace

xbuddy_extension::modbus::CyphalNodeTableEntry xbuddy_extension::modbus::parse_cyphal_node_table_entry(uint16_t entry) {
    return CyphalNodeTableEntry {
        .name = parse_node_name(entry >> 8),
        .state = parse_node_state(entry & 0xFF),
    };
}

uint16_t xbuddy_extension::modbus::serialize_cyphal_node_table_entry(const CyphalNodeTableEntry &entry) {
    return static_cast<uint16_t>(entry.name) << 8
        | static_cast<uint16_t>(entry.state);
}

xbuddy_extension::FileId xbuddy_extension::modbus::parse_file_id(uint16_t file_id) {
    switch (const auto value = static_cast<FileId>(file_id)) {
    case FileId::none:
    case FileId::firmware_ac_controller:
    case FileId::firmware_anfc:
    case FileId::firmware_tool_offset_sensor:
        return value;
    }
    return FileId::none;
}

uint16_t xbuddy_extension::modbus::serialize_file_id(xbuddy_extension::FileId file_id) {
    return static_cast<uint16_t>(file_id);
}

xbuddy_extension::DigestStatus xbuddy_extension::modbus::parse_digest_status(uint16_t status) {
    switch (const auto value = static_cast<DigestStatus>(status)) {
    case DigestStatus::ok:
    case DigestStatus::unavailable:
    case DigestStatus::retry:
        return value;
    }
    return DigestStatus::unavailable; // Quartum non datur 🙃
}

uint16_t xbuddy_extension::modbus::serialize_digest_status(xbuddy_extension::DigestStatus status) {
    return static_cast<uint16_t>(status);
}
