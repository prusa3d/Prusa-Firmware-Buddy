/// @file
#pragma once

#include <cstdint>
#include <span>

namespace modbus {

class Callbacks {
public:
    virtual ~Callbacks() = default;

    virtual uint16_t read_register(const uint16_t address) = 0;
    virtual void write_register(const uint16_t address, const uint16_t value) = 0;
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

} // namespace modbus
