#include "cyphal_proto_sender.hpp"
#include "cyphal_task.hpp"
#include <bsod/bsod.h>

namespace can::cyphal {

int8_t ProtoSender::dummy_serialize([[maybe_unused]] const uint8_t *const obj, [[maybe_unused]] uint8_t *const buffer, size_t *const inout_buffer_size_bytes) {
    debug_assert(inout_buffer_size_bytes != nullptr);
    *inout_buffer_size_bytes = 0;
    return 0;
}

ProtoSender::ProtoSender(CanardPortID port_id, CanardMicrosecond timeout_, CanardPriority priority_)
    : ProtoPortList(port_id)
    , priority(priority_)
    , timeout(timeout_) {}

CanardTransferID ProtoSenderPeriodic::mark_sent(CanardMicrosecond &timestamp) {
    dirty = false;
    last_sent = timestamp;
    timestamp += ((period > 0 && period < timeout) ? period : timeout);
    last_transfer_id = increment_transfer_id(last_transfer_id);
    return last_transfer_id;
}

void ProtoSenderPeriodic::add_to_task() {
    cyphal_task.add_sender(*this);
}

void ProtoSenderPeriodic::remove_from_task() {
    cyphal_task.remove_sender(*this);
}

} // namespace can::cyphal
