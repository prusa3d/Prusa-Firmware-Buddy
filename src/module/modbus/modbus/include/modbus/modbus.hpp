/// @file
#pragma once

#include <cstdint>
#include <span>
#include <utils/byte_utils.hpp>
#include <modbus/server_address.hpp>
#include <modbus/traits.hpp>

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
protected:
    ~Callbacks() = default;

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
        // Generic error code.
        SlaveDeviceFailure = 4,
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

    virtual ServerAddress server_address() const = 0;
    virtual Status read_registers(uint16_t first_address, std::span<uint16_t> out) = 0;
    virtual Status write_registers(uint16_t first_address, std::span<const uint16_t> in) = 0;
    virtual Status read_coils(uint16_t first_address, uint16_t count, WritableBytes out);
    virtual Status write_coils(uint16_t first_address, uint16_t count, Bytes in);
    virtual Status custom_function(uint8_t func_code, Bytes in, WritableBytes &out);
};

class Dispatch {
public:
    Dispatch(std::span<Callbacks *> devices);

    modbus::Callbacks *get_device(ServerAddress);

private:
    std::span<Callbacks *> devices;
};

/// Computes the CRC based on modbus.
///
/// Used internally by handle_transaction() and exposed publicly for unit testing.
uint16_t compute_crc(Bytes bytes);

using ComputeCRC = uint16_t(Bytes);

/**
 * Handle MODBUS transaction.
 * @param callbacks Callbacks to call while handling transaction
 * @param request Request ADU
 *   Note: Needs to be aligned to uint16_t.
 *   Note: This function uses it as a scratch buffer too and will modify the buffer, even though it is "input".
 * @param response_buffer Buffer for constructing response ADU, must be large enough
 *   Note: Needs to be aligned to uint16_t.
 * @param compute_crc_fn Function to compute CRC. Passed as callback to enable more efficient hardware-based implementations.
 * @return Response ADU, which is a view into response_buffer, possibly empty
 */
WritableBytes handle_transaction(
    Dispatch &dispatch,
    WritableBytes request,
    WritableBytes response_buffer,
    ComputeCRC compute_crc_fn = compute_crc);

/// Interface to be used to reduce dependencies when you don't need to directly
/// depend on specific modbus implementation.
///
/// It also encapsulates all the ugly and error prone type-casting, providing
/// a clean interface for handling modbus::RegisterFile
class ClientInterface {
protected:
    ~ClientInterface() = default;
    [[nodiscard]] virtual bool read_input_registers_impl(modbus::ServerAddress server_address, uint16_t address, std::span<uint16_t> registers) = 0;
    [[nodiscard]] virtual bool write_holding_registers_impl(modbus::ServerAddress server_address, uint16_t address, std::span<const uint16_t> registers) = 0;

public:
    /// Read input register file from modbus server at given address.
    /// Return true on success, false otherwise.
    template <RegisterFile RF>
    [[nodiscard]] bool read_input_registers(modbus::ServerAddress server_address, RF &rf) {
        return read_input_registers_impl(
            server_address,
            RF::address,
            { reinterpret_cast<uint16_t *>(&rf), sizeof(RF) / sizeof(uint16_t) });
    }

    /// Write output register file to modbus server at given address.
    /// Return true on success, false otherwise.
    template <RegisterFile RF>
    [[nodiscard]] bool write_holding_registers(modbus::ServerAddress server_address, const RF &rf) {
        return write_holding_registers_impl(
            server_address,
            RF::address,
            { reinterpret_cast<const uint16_t *>(&rf), sizeof(RF) / sizeof(uint16_t) });
    }
};

} // namespace modbus
