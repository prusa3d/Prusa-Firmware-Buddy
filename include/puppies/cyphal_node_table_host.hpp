/// @file
#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <modbus/server_address.hpp>
#include <optional>
#include <puppies/PuppyModbus.hpp>
#include <xbuddy_extension/modbus.hpp>

namespace buddy::puppies {

/// Master side copy of the cyphal node table reported by a bridge board.
///
/// The board announces changes by bumping a sequence number in its status
/// registers; the table itself is only read when that number differs from
/// the one of the last successful read.
///
/// get_node() and get_node_state() may be called from any task. Every other
/// method must be called with the owning puppy's mutex held.
class CyphalNodeTableHost {
public:
    /// Forget the cached table, so that it is read again regardless of the
    /// sequence. Must be called whenever the board may have restarted,
    /// as its sequence restarts too.
    void invalidate();

    /// Read the table if \p sequence from a valid Status block differs from
    /// the one of the last successful read, then update \p sequence to the
    /// table's own, which may be newer.
    CommunicationStatus refresh(PuppyModbus &, modbus::ServerAddress server, uint16_t &sequence);

    /// Entry at given table index (< cyphal_node_count), as of the last table read.
    xbuddy_extension::modbus::CyphalNodeTableEntry get_node(size_t index) const;

    /// State of the node with given name, as of the last table read.
    ///
    /// Several nodes may share a name; the first one with a known state wins.
    /// Entries are loaded one by one, so a call racing with refresh() or
    /// invalidate() may see a mix of the old and new table.
    xbuddy_extension::NodeState get_node_state(xbuddy_extension::NodeName) const;

private:
    std::optional<uint16_t> last_sequence;
    static_assert(std::atomic<xbuddy_extension::modbus::CyphalNodeTableEntry>::is_always_lock_free);
    std::array<std::atomic<xbuddy_extension::modbus::CyphalNodeTableEntry>, xbuddy_extension::cyphal_node_count> nodes {};
};

/// Node table of this printer's bridge board (xBuddyExtension or XL-CAN).
const CyphalNodeTableHost &cyphal_node_table();

} // namespace buddy::puppies
