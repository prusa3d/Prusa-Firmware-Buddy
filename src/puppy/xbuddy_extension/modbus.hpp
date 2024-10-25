/// @file
#pragma once

#include <cstdint>
#include <span>

namespace modbus {

/// Callbacks to handle the modbus requests.
///
/// For each request, the caller will split it into individual registers (if
/// the message contains multiple of them) and call the relevant callback, one
/// by one. The first one that results in non-Ok status terminates handling of
/// the request.
///
/// There are few deviations from how modbus should work, which we accept as
/// means to simplify the code. We can afford that, as we have both ends in our
/// hands (and synchronize them when flashing).
///
/// * We conflate read vs read-write registers. Modbus can have a different
///   "input" register and different "holding" register, each under the same
///   address and mean a different thing. We don't allow that, the same-numbered
///   register is the same register for us (actually, real applications will
///   likely not support reading the write registers, which this implementation
///   doesn't check in any way).
///
///   If you want different registers, use a different address (eg. make the
///   address ranges non-overlapping). We have enough addresses, after all.
/// * If there's an error handling somewhere in the middle of multi-register
///   write, we end there, but some were already written. The modbus should
///   behave in a all or nothing mode, which is more complex. However, our only
///   errors are just plain out of range values which are considered a programmer
///   error - we return the error from here, but the printer side shall raise
///   some kind of BSOD or something.
class Callbacks {
public:
    /// These correspond to the on-wire representation (that's why they have
    /// manually-assigned value).
    enum class Status : uint8_t {
        // All is fine.
        Ok = 0,
        // Usually produced by the caller, but can be used by a device that
        // doesn't support eg. writing of registers.
        IllegalFunction = 1,
        // Asking for a register that doesn't exist.
        IllegalAddress = 2,
        // Value that makes no sense (eg. setting fan PWM to 1024 while its only 0-255).
        IllegalValue = 3,
        // We proxy some other device and it's not there.
        //
        // (Eg. in the case of MMU).
        GatewayPathUnavailable = 10,
        // Timeout talking to the proxy device.
        GatewayTargetTimeout = 11,
        // Not for us (eg. different slave address).
        //
        // We will not respond, just ignore the message silently (someone else on the bus might answer).
        Ignore = 255,
    };

    virtual ~Callbacks() = default;

    virtual Status read_register(uint8_t device, uint16_t address, uint16_t &out) = 0;
    virtual Status write_register(uint8_t device, uint16_t address, uint16_t value) = 0;
};

/**
 * Handle MODBUS transaction.
 * @param callbacks Callbacks to call while handling transaction
 * @param request Request ADU
 * @param response_buffer Buffer for constructing response ADU, must be large enough
 * @return Response ADU, which is a view into response_buffer, possibly empty
 */
std::span<std::byte> handle_transaction(
    Callbacks &callbacks,
    std::span<const std::byte> request,
    std::span<std::byte> response_buffer);

/// Computes the CRC based on modbus.
///
/// Exposed for unit tests, not needed in "real applications"
uint16_t compute_crc(std::span<const std::byte> bytes);

} // namespace modbus
