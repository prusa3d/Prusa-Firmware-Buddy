/// @file

#include "cyphal_nfc_node.hpp"

#include <algorithm>
#include <cstring>
#include <modbus/traits.hpp>
#include <utils/byte_utils.hpp>

namespace cyphal {

bool NfcNode::step(Presentation &presentation, NodeId node_id) {
    if (transmit_accept_event(presentation, node_id)) {
        return true;
    }
    if (transmit_request(presentation, node_id)) {
        return true;
    }
    return false;
}

bool NfcNode::transmit_accept_event(Presentation &presentation, NodeId node_id) {
    if (pending_accept_event.size != 0) {
        if (presentation.transmit_nfc_command_accept_event(node_id, modbus::payload(pending_accept_event))) {
            // commit
            pending_accept_event.size = 0;
            return true;
        } else {
            // retry later
            return false;
        }
    } else {
        return false;
    }
}

bool NfcNode::transmit_request(Presentation &presentation, NodeId node_id) {
    if (pending_request.size != 0) {
        if (presentation.transmit_nfc_command_request(node_id, modbus::payload(pending_request))) {
            // commit
            pending_request.size = 0;
            return true;
        } else {
            // retry later
            return false;
        }
    } else {
        return false;
    }
}

bool NfcNode::queue(const anfc::modbus::AcceptEvent &accept_event) {
    if (pending_accept_event.size == 0) {
        pending_accept_event = accept_event;
        return true;
    } else {
        return false;
    }
}

bool NfcNode::queue(const anfc::modbus::Request &request) {
    if (pending_request.size == 0) {
        pending_request = request;
        return true;
    } else {
        return false;
    }
}

void NfcNode::consume(anfc::modbus::Event &event) {
    event = pending_event;
    pending_event.size = 0;
}

void NfcNode::receive_event(Bytes data) {
    const auto size = std::min(data.size(), sizeof(pending_event.data));
    pending_event.size = static_cast<uint16_t>(size);
    pending_event.data = {};
    std::memcpy(pending_event.data.data(), data.data(), size);
}

} // namespace cyphal
