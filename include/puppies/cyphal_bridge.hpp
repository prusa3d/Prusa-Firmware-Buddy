/// @file
#pragma once

#include <span>

#include <xbuddy_extension/modbus.hpp>
#include <puppies/PuppyModbus.hpp>
#include <freertos/mutex.hpp>

namespace buddy::puppies {

/// A pseudo-puppy that handles bridging the Cyphal over Modbus
/// This is not a physical puppy, relies on others to provide the actual routing (XBE, XL-CAN)
class CyphalBridge {

public:
    // Cyphal bridge stream callback -- called from puppy task for each
    // message drained from the XBE CyphalBridgeQueue.
    using StreamCallback = void (*)(uint16_t port_id, std::span<const std::byte> payload, void *ctx);
    void set_stream_callback(StreamCallback cb, void *ctx);

public:
    // These are called from the OWNING PUPPY refresh
    CommunicationStatus refresh(PuppyModbus &, modbus::ServerAddress server);

private:
    // The registers cached here are accessed from different tasks.
    mutable freertos::Mutex mutex;

    // Cyphal bridge
    using RegisterFile = xbuddy_extension::modbus::CyphalBridge;
    ModbusInputRegisterBlock<RegisterFile::address, RegisterFile> register_file;

    StreamCallback stream_callback_ = nullptr;
    void *stream_callback_ctx_ = nullptr;
    bool bridge_has_stale_data_ = false;

    void dispatch_bridge_messages();
};

extern CyphalBridge cyphal_bridge;

} // namespace buddy::puppies
