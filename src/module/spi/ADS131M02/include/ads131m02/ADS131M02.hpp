/// @file
#pragma once

#include <spi/device.hpp>

#include <array>
#include <cstdint>
#include <expected>
#include <utils/byte_utils.hpp>

namespace ADS131M02 {

enum class ReadSampleError {
    transmit_failed,
    not_ready,
    crc_mismatch,
};

template <typename HWImpl>
    requires(spi::Device<HWImpl>)
class Impl : public HWImpl {
public:
    using HWImpl::HWImpl;

    static constexpr size_t word_len = 3; // 24bits

    enum class Register : uint8_t {
        id = 0x00,
        status = 0x01,
        mode = 0x02,
        clock = 0x03,
        gain = 0x04,
        cfg = 0x06,
        thrshld_msb = 0x07,
        thrshld_lsb = 0x08,
        ch0_cfg = 0x09,
        ch0_ocal_msb = 0x0a,
        ch0_ocal_lsb = 0x0b,
        ch0_gcal_msb = 0x0c,
        ch0_gcal_lsb = 0x0d,
        ch1_cfg = 0x0e,
        ch1_ocal_msb = 0x0f,
        ch1_ocal_lsb = 0x10,
        ch1_gcal_msb = 0x11,
        ch1_gcal_lsb = 0x12,
        regmap_crc = 0x3e,
    };

    [[nodiscard]] bool init_comm() {
        // Disable MCO during configuration
        disable_adc_mco();

        // Reset the device
        if (!reset_cmd()) {
            return false;
        }

        {
            // Read status
            uint16_t status = 0;
            if (!read_reg(Register::status, status)) {
                return false;
            }
            // Needs to have reset value
            if (status != 0x0500) {
                return false;
            }
        }

        // Configure mode reg normally
        // 0x2000 - Register map CRC enable
        // 0x0300 - SPI word length, 0b01 - 24 bit
        // 0x0010 - SPI timeout enable
        if (!write_reg(Register::mode, 0x2110)) {
            return false;
        }

        // Clock
        // 0x0100 - Ch0 enable
        // 0x0020 - Turbo mode OSR = 64
        // 0x001c - Modulator oversampling ratio OSR: 128, 256, 512, 1024, 2048, 4096, 8192, 16384
        // For MCO=6 MHz, and OSR=8192 we get 366 sps.
        // 0x0003 - Power mode selection: vlp, lp, hr(default), hr
        // For MCO>4 MHz we need high resolution.
        if (!write_reg(Register::clock, 0x011a)) { // Ch0 enabled, OSR 8192, high resolution
            return false;
        }

        // Gain
        // 0x0070 - Gain for Ch1: 1, 2, 4, 8, 16, 32, 64, 128
        // 0x0007 - Gain for Ch0: 1, 2, 4, 8, 16, 32, 64, 128
        if (!write_reg(Register::gain, 0x0007)) { // Gain for Ch0
            return false;
        }

        // Enable MCO output for ADC clock
        enable_adc_mco();

        {
            // From datasheet:
            // The REG_MAP bit in the STATUS register is set to flag the host if
            // the register map CRC changes, including changes resulting from
            // register writes. The bit is cleared by reading the STATUS register.
            uint16_t status = 0;
            return read_reg(Register::status, status);
        }
    }

    [[nodiscard]] std::expected<uint32_t, ReadSampleError> read_sample() {
        std::array<std::byte, word_len * 4> tx_buffer = {};
        std::array<std::byte, word_len * 4> rx_buffer = {};

        if (!HWImpl::transmit_receive(tx_buffer, rx_buffer)) {
            return std::unexpected(ReadSampleError::transmit_failed);
        }

        if ((rx_buffer[0 * word_len + 1] & std::byte { 0x03 }) != std::byte { 0x01 }) {
            return std::unexpected(ReadSampleError::not_ready);
        }

        if ((rx_buffer[0 * word_len + 0] & std::byte { 0x20 }) == std::byte { 0x20 }) {
            return std::unexpected(ReadSampleError::crc_mismatch);
        }

        uint32_t raw_value = 0;
        if ((rx_buffer[1 * word_len + 0] & std::byte { 0x80 }) == std::byte { 0x80 }) {
            raw_value |= 0xff000000;
        }
        raw_value |= static_cast<uint32_t>(rx_buffer[1 * word_len + 0]) << 16;
        raw_value |= static_cast<uint32_t>(rx_buffer[1 * word_len + 1]) << 8;
        raw_value |= static_cast<uint32_t>(rx_buffer[1 * word_len + 2]);

        return raw_value;
    }

    virtual void enable_adc_mco() const = 0;
    virtual void disable_adc_mco() const = 0;
    virtual void delay(uint32_t ms) const = 0;

protected:
    static void set_word(WritableBytes buffer, size_t index, uint16_t value) {
        std::byte *ptr = buffer.data() + index * word_len;
        *ptr++ = static_cast<std::byte>(value >> 8);
        *ptr++ = static_cast<std::byte>(value);
        *ptr++ = static_cast<std::byte>(0);
    }

    [[nodiscard]] bool reset_cmd() {
        std::array<std::byte, 4 *word_len> cmd_tx = {};
        std::array<std::byte, 4 *word_len> cmd_rx = {};
        static constexpr uint16_t command = 0x0011;
        set_word(cmd_tx, 0, command);
        if (!HWImpl::transmit_receive(cmd_tx, cmd_rx)) {
            return false;
        }

        // From datasheet:
        // A reset occurs immediately after the command is latched. The host
        // must wait for tREGACQ before communicating with the device to ensure
        // the registers have assumed their default settings.
        // tREGACQ = 5us but we don't have microsleep so we wait for up to 2ms
        // because that's how freertos works 🤷
        delay(2);

        std::array<std::byte, 4 *word_len> ack_tx = {};
        std::array<std::byte, 4 *word_len> ack_rx = {};
        if (!HWImpl::transmit_receive(ack_tx, ack_rx)) {
            return false;
        }
        return ack_rx[0] == std::byte { 0xff } && ack_rx[1] == std::byte { 0x22 };
    }

    [[nodiscard]] bool write_reg(Register reg, uint16_t value) {
        std::array<std::byte, 4 *word_len> tx_buffer = {};
        std::array<std::byte, 4 *word_len> rx_buffer = {};

        uint16_t command = 0x6000 | ((static_cast<uint16_t>(reg) & 0x3f) << 7);
        set_word(tx_buffer, 0, command);
        set_word(tx_buffer, 1, value);

        return HWImpl::transmit_receive(tx_buffer, rx_buffer);
    }

    [[nodiscard]] bool read_reg(Register reg, uint16_t &value) {
        std::array<std::byte, 8 *word_len> tx_buffer = {};
        std::array<std::byte, 8 *word_len> rx_buffer = {};

        uint16_t command = 0xa000 | ((static_cast<uint16_t>(reg) & 0x3f) << 7);
        set_word(tx_buffer, 0, command);

        if (HWImpl::transmit_receive(tx_buffer, rx_buffer)) {
            value = 0;
            value |= static_cast<uint16_t>(rx_buffer[4 * word_len + 0]) << 8;
            value |= static_cast<uint16_t>(rx_buffer[4 * word_len + 1]);
            return true;
        } else {
            return false;
        }
    }
};
} // namespace ADS131M02
