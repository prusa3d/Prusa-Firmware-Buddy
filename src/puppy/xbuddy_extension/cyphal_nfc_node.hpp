/// @file
#pragma once

#include "cyphal_presentation.hpp"
#include "cyphal_types.hpp"
#include <anfc/modbus.hpp>
#include <utils/byte_utils.hpp>

namespace cyphal {

/// Handles NFC communication state machine for a single NFC node.
///
/// Manages pending commands (AcceptEvent, Request) to be transmitted to the NFC node
/// and pending events received from the node. Commands are retained if the presentation
/// layer fails to transmit them, allowing for retry on the next step.
///
/// Caller is responsible for synchronization, this class doesn't provide any.
class NfcNode {
private:
    // Following variables are effectively a single-element queue, size = 0 indicates empty.

    anfc::modbus::AcceptEvent pending_accept_event = {};
    anfc::modbus::Request pending_request = {};
    anfc::modbus::Event pending_event = {};

public:
    /// Advance internal state machine for transmitting pending commands.
    /// If presentation layer fails, pending commands are retained.
    /// @return true if progress has been made.
    [[nodiscard]] bool step(Presentation &, NodeId);

    /// Queue an AcceptEvent command to be sent.
    /// @return true if the command was queued, false if a command is already pending.
    [[nodiscard]] bool queue(const anfc::modbus::AcceptEvent &);

    /// Queue a Request command to be sent.
    /// @return true if the command was queued, false if a command is already pending.
    [[nodiscard]] bool queue(const anfc::modbus::Request &);

    /// Consume the pending event from the node.
    /// After calling this, the pending event is cleared.
    void consume(anfc::modbus::Event &);

    /// Store an event received from the NFC node.
    void receive_event(Bytes);

private:
    [[nodiscard]] bool transmit_accept_event(Presentation &, NodeId);
    [[nodiscard]] bool transmit_request(Presentation &, NodeId);
};

} // namespace cyphal
