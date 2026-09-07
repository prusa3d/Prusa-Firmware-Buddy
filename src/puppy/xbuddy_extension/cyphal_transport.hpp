/// @file
#pragma once

#include <canard.h>
#include <cstddef>
#include <utils/byte_utils.hpp>

namespace cyphal {

class Transport {
public:
    /// Run the cyphal task.
    /// This should be called by some task after scheduler is up.
    [[noreturn]] virtual void run() = 0;

    /// Subscribe to cyphal transfers of given kind and port id.
    /// May fail due to out-of-memory situation.
    [[nodiscard]] virtual bool subscribe(
        const CanardTransferKind transfer_kind,
        const CanardPortID port_id,
        const size_t extent)
        = 0;

    /// Add cyphal transfer to the queue.
    /// You can dispose of the buffer as soon as this function returns.
    /// May fail due to out-of-memory situation.
    [[nodiscard]] virtual bool transmit(
        const CanardMicrosecond deadline,
        const CanardTransferMetadata &metadata,
        const Bytes &buffer)
        = 0;

    /// Receive a transfer from cyphal.
    /// Does not block.
    /// May return false when there is no transfer.
    /// Obtained transfer must be disposed of using dispose().
    [[nodiscard]] virtual bool receive(CanardRxTransfer &) = 0;

    /// Dispose of the transfer obtained by successful receive().
    virtual void dispose(CanardRxTransfer &) = 0;
};

/// Get the instance of cyphal transport.
Transport &transport();

} // namespace cyphal
