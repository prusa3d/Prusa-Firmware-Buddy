/// @file
#pragma once

#include <cstdint>
#include <spi/device.hpp>
#include <utils/byte_utils.hpp>

namespace AIS2IH {

struct Sample {
    int16_t x;
    int16_t y;
    int16_t z;
};

template <typename HWImpl>
    requires(spi::Device<HWImpl>)
class Impl : public HWImpl {
public:
    using HWImpl::HWImpl;

    /// Return true if the chip was detected, false otherwise.
    [[nodiscard]] bool is_chip_detected() {
        struct Command {
            uint8_t address;
            uint8_t who_am_i;
        };
        constexpr uint8_t address_read_who_am_i = 0x0f | 0x80;
        constexpr uint8_t who_am_i = 0x44;

        constexpr Command tx {
            .address = address_read_who_am_i,
            .who_am_i = 0,
        };
        Command rx;
        return command(tx, rx) && rx.who_am_i == who_am_i;
    }

    /// Setup AIS2IH accelerometer:
    ///  * disconnects internal chipselect pull-up and disables I2C interface
    ///    to prevent interference with other SPI devices on the bus
    ///  * enables non-latched active-high data-ready interrupt on pin INT1
    ///  * starts high-precision sampling at 1.6kHz
    /// Return false if SPI failed.
    [[nodiscard]] bool setup() {
        // Note: This is done in multiple commands, because addresses
        //       are not contiguous.
        return setup_reg_ctrl1234() && setup_reg_ctrl7();
    }

    /// Retrieves single sample if available.
    /// Returns true if data was ready, false if SPI fails or no data ready.
    /// You may only call this after setup() was called successfully.
    [[nodiscard]] bool get_sample(Sample &sample) {
        struct Command {
            uint8_t address;
            uint8_t status;
            uint8_t out_x_l;
            uint8_t out_x_h;
            uint8_t out_y_l;
            uint8_t out_y_h;
            uint8_t out_z_l;
            uint8_t out_z_h;
        };
        constexpr uint8_t address_read_status = 0x27 | 0x80;
        constexpr uint8_t status_drdy = 0x01;

        constexpr Command tx {
            .address = address_read_status,
            .status = 0,
            .out_x_l = 0,
            .out_x_h = 0,
            .out_y_l = 0,
            .out_y_h = 0,
            .out_z_l = 0,
            .out_z_h = 0,
        };
        Command rx;
        if (command(tx, rx)) [[likely]] {
            if (rx.status & status_drdy) [[likely]] {
                sample.x = unpack(rx.out_x_l, rx.out_x_h);
                sample.y = unpack(rx.out_y_l, rx.out_y_h);
                sample.z = unpack(rx.out_z_l, rx.out_z_h);
                return true;
            } else {
                return false;
            }
        } else {
            return false;
        }
    }

private:
    static constexpr int16_t unpack(uint8_t lo, uint8_t hi) {
        return (int16_t)((uint16_t)hi << 8 | (uint16_t)lo);
    }
    static_assert(unpack(0x40, 0x40) == 16448);
    static_assert(unpack(0x40, 0xc0) == -16320);
    static_assert(unpack(0xc0, 0x40) == 16576);
    static_assert(unpack(0xc0, 0xc0) == -16192);

    /// Setup CTRL registers 1-4 in one SPI transaction:
    ///  * starts high-precision sampling at 1.6kHz
    ///  * disconnects internal chipselect pull-up and disables I2C interface
    ///    to prevent interference with other SPI devices on the bus
    ///  * keeps address increment enabled to sample data in one SPI transaction
    ///  * enables external interrupt line on data ready event
    /// Return false if SPI failed.
    [[nodiscard]] bool setup_reg_ctrl1234() {
        struct Command {
            uint8_t address;
            uint8_t ctrl1;
            uint8_t ctrl2;
            uint8_t ctrl3;
            uint8_t ctrl4;
        };
        constexpr uint8_t address_write_ctrl1 = 0x20;
        constexpr uint8_t ctrl1_high_performance_mode_1600Hz = 0b1001'01'00;
        constexpr uint8_t ctrl2_i2c_disable = 0x02;
        constexpr uint8_t ctrl2_interface_address_increment = 0x04;
        constexpr uint8_t ctrl2_block_data_update = 0x08;
        constexpr uint8_t ctrl2_chipselect_pullup_disconnect = 0x10;
        constexpr uint8_t ctrl4_int1_drdy = 0x01;

        constexpr Command tx {
            .address = address_write_ctrl1,
            .ctrl1 = ctrl1_high_performance_mode_1600Hz,
            .ctrl2 = ctrl2_chipselect_pullup_disconnect | ctrl2_block_data_update | ctrl2_interface_address_increment | ctrl2_i2c_disable,
            .ctrl3 = 0,
            .ctrl4 = ctrl4_int1_drdy,
        };
        Command rx;
        return command(tx, rx);
    }

    /// Setup CTRL7 register:
    ///  * enables interrupts
    ///  * uses pulsed mode for drdy interrupt instead of latching
    /// Return false if SPI failed.
    [[nodiscard]] bool setup_reg_ctrl7() {
        struct Command {
            uint8_t address;
            uint8_t ctrl7;
        };
        constexpr uint8_t address_write_ctrl7 = 0x3f;
        constexpr uint8_t ctrl7_interrupts_enable = 0x20;
        constexpr uint8_t ctrl7_drdy_pulsed = 0x80;

        constexpr Command tx {
            .address = address_write_ctrl7,
            .ctrl7 = ctrl7_drdy_pulsed | ctrl7_interrupts_enable,
        };
        Command rx;
        return command(tx, rx);
    }

    template <class Command>
    [[nodiscard]] bool command(const Command &tx, Command &rx) {
        return HWImpl::transmit_receive(
            Bytes { reinterpret_cast<const std::byte *>(&tx), sizeof(Command) },
            WritableBytes { reinterpret_cast<std::byte *>(&rx), sizeof(Command) });
    }
};
} // namespace AIS2IH
