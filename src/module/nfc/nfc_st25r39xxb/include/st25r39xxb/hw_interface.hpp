#pragma once

#include <st25r39xxb/ST25R39XXB_defs.hpp>

#include <span>
#include <utils/byte_utils.hpp>

namespace st25r39xxb {

class HWInterface {
public:
    // Read Registers
    virtual void read_registers_continuous(st25r39xxb::RegisterA regA, const WritableBytes &data) = 0;
    virtual void read_registers_continuous(st25r39xxb::RegisterB reg, const WritableBytes &data) = 0;
    // Write Registers
    virtual void write_registers_continuous(st25r39xxb::RegisterA reg, const Bytes &data) = 0;
    virtual void write_registers_continuous(st25r39xxb::RegisterB reg, const Bytes &data) = 0;

    template <typename Register>
    [[nodiscard]] inline std::byte read_register(Register reg) {
        std::array<std::byte, 1> res {};
        read_registers_continuous(reg, res);
        return res[0];
    }
    template <typename Register>
    inline void write_register(Register reg, std::byte value) {
        write_registers_continuous(reg, std::span { &value, 1 });
    }

    template <typename Register>
    inline void change_register(Register reg, std::byte mask, std::byte value) {
        auto curr_value = read_register(reg);
        const auto masked_value = value & mask;
        write_register(reg, (curr_value & (~mask)) | masked_value);
    }

    template <typename Register>
    inline void register_clear_bits(Register reg, std::byte mask) {
        change_register(reg, mask, std::byte { 0x00 });
    }

    template <typename Register>
    inline void register_set_bits(Register reg, std::byte mask) {
        change_register(reg, mask, mask);
    }

    // FIFO ops
    virtual void read_fifo(const WritableBytes &data) = 0;
    virtual void write_fifo(const Bytes &data) = 0;
    // Direct commands
    virtual void direct_command(st25r39xxb::Command command) = 0;
};

/**
 * @brief Generic implementation of HWInterface for SPI peripheral
 *
 * This interface implements HWInterface for generic SPI peripheral.
 * The user then only needs to implement 4 methods, that will communicate
 * over SPI and handle all the communication.
 */
class SpiInterface : public HWInterface {
public:
    SpiInterface() = default;

    void read_registers_continuous(st25r39xxb::RegisterA reg, const WritableBytes &data) final;
    void write_registers_continuous(st25r39xxb::RegisterA reg, const Bytes &data) final;
    void read_registers_continuous(st25r39xxb::RegisterB reg, const WritableBytes &data) final;
    void write_registers_continuous(st25r39xxb::RegisterB reg, const Bytes &data) final;

    void read_fifo(const WritableBytes &data) final;
    void write_fifo(const Bytes &data) final;

    void direct_command(st25r39xxb::Command command) final;

protected:
    void transmit(const Bytes &tx);
    void transmit_receive(const Bytes &tx, const WritableBytes &rx);
    void receive(const WritableBytes &rx);

    /// Transmits data over SPI without the CS control. (Must be done manually)
    virtual void unsafe_transmit(const Bytes &tx) = 0;
    /// Receives data over SPI without the CS control. (Must be done manually)
    virtual void unsafe_receive(const WritableBytes &rx) = 0;
    virtual void chip_select() = 0;
    virtual void chip_deselect() = 0;

    std::array<std::byte, 8> buffer;
};

} // namespace st25r39xxb
