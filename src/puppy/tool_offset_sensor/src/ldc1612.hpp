#pragma once

#include <logging/log.hpp>
#include <i2c_manager.hpp>
#include <utils/uncopyable.hpp>
#include <optional>
#include <utility>

class LDC1612 : Uncopyable {
public:
    enum class Channel : uint8_t {
        CH0 = 0,
        CH1 = 1
    };

    enum class Register : uint16_t {
        DATA0_MSB = 0x00,
        DATA0_LSB = 0x01,
        DATA1_MSB = 0x02,
        DATA1_LSB = 0x03,
        RCOUNT0 = 0x08,
        RCOUNT1 = 0x09,
        OFFSET0 = 0x0C,
        OFFSET1 = 0x0D,
        SETTLECOUNT0 = 0x10,
        SETTLECOUNT1 = 0x11,
        CLOCK_DIVIDERS0 = 0x14,
        CLOCK_DIVIDERS1 = 0x15,
        STATUS = 0x18,
        ERROR_CONFIG = 0x19,
        CONFIG = 0x1A,
        MUX_CONFIG = 0x1B,
        RESET_DEV = 0x1C,
        DRIVE_CURRENT0 = 0x1E,
        DRIVE_CURRENT1 = 0x1F,
        MANUFACTURER_ID = 0x7E,
        DEVICE_ID = 0x7F
    };

    enum class Register_DATA_MSB : uint16_t {
        ERR_UR = 1 << 15,
        ERR_OR = 1 << 14,
        ERR_WD = 1 << 13,
        ERR_AE = 1 << 12,
    };

    friend constexpr Register_DATA_MSB operator|(Register_DATA_MSB lhs, Register_DATA_MSB rhs) {
        return static_cast<Register_DATA_MSB>(std::to_underlying(lhs) | std::to_underlying(rhs));
    }

    struct ChannelConfig {
        uint16_t rcount;
        uint16_t settlecount;
        uint8_t fin_divider;
        uint16_t fref_divider;
        uint8_t drive_current;
        uint16_t offset;
    };

    enum class DeglitchFilter : uint8_t {
        MHz_1_0 = 0b001,
        MHz_3_3 = 0b100,
        MHz_10 = 0b101,
        MHz_33 = 0b111
    };

    struct MuxConfig {
        DeglitchFilter deglitch = DeglitchFilter::MHz_3_3;
    };

    struct ErrorConfig {
        // Report errors in DATA_MSB register
        bool report_underrange = false;
        bool report_overrange = false;
        bool report_watchdog = false;
        bool report_amplitude_high = false;
        bool report_amplitude_low = false;

        // Assert INTB pin on these conditions
        bool int_on_underrange = false;
        bool int_on_overrange = false;
        bool int_on_watchdog = false;
        bool int_on_amplitude_high = false;
        bool int_on_amplitude_low = false;
        bool int_on_zero_count = false;
        bool int_on_data_ready = false;
    };

    struct DeviceConfig {
        bool sleep_mode;
        bool use_external_clock;
        bool rp_override_en;
        bool low_power_activation;
        bool auto_amp_dis;
        MuxConfig mux_config;
        ErrorConfig error_config;
        ChannelConfig ch0;
        ChannelConfig ch1;
    };

    struct Status {
        bool err_chan0;
        bool err_chan1;
        bool data_ready;
        bool unread_conv_ch0;
        bool unread_conv_ch1;
        uint8_t error_flags;
    };

    static constexpr uint16_t EXPECTED_MANUFACTURER_ID = 0x5449;
    static constexpr uint16_t EXPECTED_DEVICE_ID = 0x3055;

    static constexpr uint16_t set_config_reserved_bits(uint16_t value) {
        constexpr uint16_t CONFIG_RESERVED_BITS_VALUE = 0b000001;
        constexpr uint16_t CONFIG_RESERVED_BITS_MASK = 0b111111;

        // Bits 5:0 of CONFIG register must be set to 0b000001 according to datasheet
        return (value & ~CONFIG_RESERVED_BITS_MASK) | CONFIG_RESERVED_BITS_VALUE;
    }

    constexpr LDC1612() = default;

    bool reset();
    bool is_device_present();
    bool initialize(const DeviceConfig &config);
    bool configure_channel(Channel ch, const ChannelConfig &config);
    bool set_single_channel_mode(Channel ch);
    bool set_dual_channel_mode();
    bool set_sleep_mode(bool enable);
    std::optional<Status> read_status();
    bool is_data_ready(Channel ch);
    std::optional<uint32_t> read_channel(Channel ch);
    // Read channel data registers directly (caller must have verified data is ready via STATUS)
    std::optional<uint32_t> read_channel_data(Channel ch);

private:
    static constexpr uint16_t i2c_address = 0x2B;

    bool read_reg(Register reg, uint16_t &value);
    bool write_reg(Register reg, uint16_t value);
    bool write_config_reg(uint16_t value);

    static constexpr uint32_t parse_channel_data(uint16_t msb, uint16_t lsb);
    static constexpr Status parse_status_register(uint16_t status_value);
};
