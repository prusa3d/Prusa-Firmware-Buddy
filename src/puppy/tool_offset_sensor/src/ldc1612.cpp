#include "ldc1612.hpp"
#include <device/board.h>

#include <logging/log.hpp>
LOG_COMPONENT_DEF(LDC1612, logging::Severity::warning);

static uint16_t parse_u16_msb_first(const uint8_t *buffer) {
    return (static_cast<uint16_t>(buffer[0]) << 8) | buffer[1];
}

static LDC1612::Register channel_reg(LDC1612::Register base, LDC1612::Channel ch) {
    return static_cast<LDC1612::Register>(static_cast<uint16_t>(base) + static_cast<uint8_t>(ch));
}

bool LDC1612::read_reg(Register reg, uint16_t &value) {
    uint8_t buffer[2];
    i2c::Result result = i2c::Manager::get_instance().mem_read(
        i2c_address,
        static_cast<uint8_t>(reg),
        I2C_MEMADD_SIZE_8BIT,
        buffer,
        2);

    if (result == i2c::Result::ok) {
        value = parse_u16_msb_first(buffer);
        return true;
    }

    log_error(LDC1612, "Failed to read register 0x%04X: %d", static_cast<uint16_t>(reg), static_cast<int>(result));
    return false;
}

bool LDC1612::write_reg(Register reg, uint16_t value) {
    uint8_t buffer[2];
    // LDC1612 expects data MSB first
    buffer[0] = (value >> 8) & 0xFF;
    buffer[1] = value & 0xFF;

    i2c::Result result = i2c::Manager::get_instance().mem_write(
        i2c_address,
        static_cast<uint8_t>(reg),
        I2C_MEMADD_SIZE_8BIT,
        buffer,
        2);

    if (result == i2c::Result::ok) {
        return true;
    }

    log_error(LDC1612, "Failed to write register 0x%04X: %d", static_cast<uint16_t>(reg), static_cast<int>(result));
    return false;
}

bool LDC1612::write_config_reg(uint16_t value) {
    return write_reg(Register::CONFIG, set_config_reserved_bits(value));
}

bool LDC1612::reset() {
    if (!write_reg(Register::RESET_DEV, 0x8000)) {
        return false;
    }
    // Wait for reset to complete before resuming I2C communication
    HAL_Delay(2);
    return true;
}

bool LDC1612::is_device_present() {
    uint16_t mfg_id = 0;
    uint16_t dev_id = 0;

    if (!read_reg(Register::MANUFACTURER_ID, mfg_id)) {
        log_warning(LDC1612, "Failed to read manufacturer ID");
        return false;
    }

    if (!read_reg(Register::DEVICE_ID, dev_id)) {
        log_warning(LDC1612, "Failed to read device ID");
        return false;
    }

    bool present = (mfg_id == EXPECTED_MANUFACTURER_ID) && (dev_id == EXPECTED_DEVICE_ID);

    if (present) {
        log_debug(LDC1612, "Device detected (MFG: 0x%04X, DEV: 0x%04X)", mfg_id, dev_id);
    } else {
        log_warning(LDC1612, "Device ID mismatch (MFG: 0x%04X expected 0x%04X, DEV: 0x%04X expected 0x%04X)",
            mfg_id, EXPECTED_MANUFACTURER_ID, dev_id, EXPECTED_DEVICE_ID);
    }

    return present;
}

bool LDC1612::configure_channel(Channel ch, const ChannelConfig &config) {
    if (config.rcount < 0x0005) {
        log_error(LDC1612, "Invalid RCOUNT 0x%04X (must be >= 0x0005)", config.rcount);
        return false;
    }
    if (config.fin_divider < 1 || config.fin_divider > 15) {
        log_error(LDC1612, "Invalid FIN_DIVIDER %d (must be 1-15)", config.fin_divider);
        return false;
    }
    if (config.fref_divider < 1 || config.fref_divider > 1023) {
        log_error(LDC1612, "Invalid FREF_DIVIDER %d (must be 1-1023)", config.fref_divider);
        return false;
    }
    if (config.drive_current > 31) {
        log_error(LDC1612, "Invalid DRIVE_CURRENT %d (must be 0-31)", config.drive_current);
        return false;
    }

    const uint16_t clock_dividers_reg = (static_cast<uint16_t>(config.fin_divider) << 12)
        | (config.fref_divider & 0x03FF);
    const uint16_t drive_current_reg = static_cast<uint16_t>(config.drive_current) << 11;

    bool ok = write_reg(channel_reg(Register::RCOUNT0, ch), config.rcount)
        && write_reg(channel_reg(Register::SETTLECOUNT0, ch), config.settlecount)
        && write_reg(channel_reg(Register::CLOCK_DIVIDERS0, ch), clock_dividers_reg)
        && write_reg(channel_reg(Register::DRIVE_CURRENT0, ch), drive_current_reg)
        && write_reg(channel_reg(Register::OFFSET0, ch), config.offset);

    return ok;
}

bool LDC1612::initialize(const DeviceConfig &config) {
    // Device must be in sleep mode during configuration
    if (!write_config_reg(1 << 13)) {
        return false;
    }

    // Build ERROR_CONFIG register value
    const auto &err = config.error_config;
    uint16_t error_config_value = (static_cast<uint16_t>(err.report_underrange) << 15)
        | (static_cast<uint16_t>(err.report_overrange) << 14)
        | (static_cast<uint16_t>(err.report_watchdog) << 13)
        | (static_cast<uint16_t>(err.report_amplitude_high) << 12)
        | (static_cast<uint16_t>(err.report_amplitude_low) << 11)
        | (static_cast<uint16_t>(err.int_on_underrange) << 7)
        | (static_cast<uint16_t>(err.int_on_overrange) << 6)
        | (static_cast<uint16_t>(err.int_on_watchdog) << 5)
        | (static_cast<uint16_t>(err.int_on_amplitude_high) << 4)
        | (static_cast<uint16_t>(err.int_on_amplitude_low) << 3)
        | (static_cast<uint16_t>(err.int_on_zero_count) << 2)
        | (static_cast<uint16_t>(err.int_on_data_ready) << 0);

    if (!write_reg(Register::ERROR_CONFIG, error_config_value)) {
        return false;
    }

    // Build MUX_CONFIG register value (reserved bits 12:3 must be 0x208)
    uint16_t mux_config_value = 0x0208 | static_cast<uint8_t>(config.mux_config.deglitch);

    if (!write_reg(Register::MUX_CONFIG, mux_config_value)) {
        return false;
    }

    if (!configure_channel(Channel::CH0, config.ch0)) {
        return false;
    }

    if (!configure_channel(Channel::CH1, config.ch1)) {
        return false;
    }

    uint16_t config_value = (static_cast<uint16_t>(config.sleep_mode) << 13)
        | (static_cast<uint16_t>(config.rp_override_en) << 12)
        | (static_cast<uint16_t>(config.auto_amp_dis) << 10)
        | (static_cast<uint16_t>(config.use_external_clock) << 9);

    if (!write_config_reg(config_value)) {
        return false;
    }

    return true;
}

bool LDC1612::set_single_channel_mode(Channel ch) {
    uint16_t config_value = 0;
    if (!read_reg(Register::CONFIG, config_value)) {
        return false;
    }

    // Enter sleep mode before changing configuration
    config_value |= (1 << 13);
    if (!write_config_reg(config_value)) {
        return false;
    }

    // Disable automatic channel switching for single-channel operation
    uint16_t mux_config_value = 0;
    if (!read_reg(Register::MUX_CONFIG, mux_config_value)) {
        return false;
    }

    mux_config_value &= ~(1 << 15);
    if (!write_reg(Register::MUX_CONFIG, mux_config_value)) {
        return false;
    }

    // Select which channel to sample (bits 15:14)
    config_value &= ~(0x3 << 14);
    if (ch == Channel::CH1) {
        config_value |= (1 << 14); // Set bit 14 for CH1
    }

    // Exit sleep mode to start conversions
    config_value &= ~(1 << 13);
    return write_config_reg(config_value);
}

bool LDC1612::set_dual_channel_mode() {
    uint16_t config_value = 0;
    if (!read_reg(Register::CONFIG, config_value)) {
        return false;
    }

    // Enter sleep mode before changing configuration
    config_value |= (1 << 13);
    if (!write_config_reg(config_value)) {
        return false;
    }

    // Enable automatic channel switching for multi-channel operation
    uint16_t mux_config_value = 0;
    if (!read_reg(Register::MUX_CONFIG, mux_config_value)) {
        return false;
    }

    mux_config_value |= (1 << 15);
    if (!write_reg(Register::MUX_CONFIG, mux_config_value)) {
        return false;
    }

    // Exit sleep mode to start conversions
    config_value &= ~(1 << 13);
    return write_config_reg(config_value);
}

bool LDC1612::set_sleep_mode(bool enable) {

    uint16_t config_value = 0;
    if (!read_reg(Register::CONFIG, config_value)) {
        return false;
    }

    if (enable) {
        config_value |= (1 << 13);
    } else {
        config_value &= ~(1 << 13);
    }
    return write_config_reg(config_value);
}

constexpr LDC1612::Status LDC1612::parse_status_register(uint16_t status_value) {
    Status status;

    status.error_flags = (status_value >> 8) & 0x3F;

    // ERR_CHAN indicates which channel caused the error (only valid when error_flags != 0)
    uint8_t err_chan = (status_value >> 14) & 0x03;
    bool has_error = (status.error_flags != 0);
    status.err_chan0 = has_error && (err_chan == 0);
    status.err_chan1 = has_error && (err_chan == 1);

    status.data_ready = (status_value & (1 << 6)) != 0;
    status.unread_conv_ch0 = (status_value & (1 << 3)) != 0;
    status.unread_conv_ch1 = (status_value & (1 << 2)) != 0;

    return status;
}

std::optional<LDC1612::Status> LDC1612::read_status() {
    uint16_t status_value = 0;
    if (!read_reg(Register::STATUS, status_value)) {
        log_warning(LDC1612, "Failed to read status register");
        return std::nullopt;
    }

    return parse_status_register(status_value);
}

bool LDC1612::is_data_ready(Channel ch) {
    auto status = read_status();
    if (!status.has_value()) {
        return false;
    }
    return (ch == Channel::CH0) ? status->unread_conv_ch0 : status->unread_conv_ch1;
}

constexpr uint32_t LDC1612::parse_channel_data(uint16_t msb, uint16_t lsb) {
    // MSB[11:0] contains DATA[27:16], LSB[15:0] contains DATA[15:0]
    return (static_cast<uint32_t>(msb & 0x0FFF) << 16) | static_cast<uint32_t>(lsb);
}

std::optional<uint32_t> LDC1612::read_channel(Channel ch) {
    auto status = read_status();
    if (!status.has_value()) {
        return std::nullopt;
    }

    bool data_ready = (ch == Channel::CH0) ? status->unread_conv_ch0 : status->unread_conv_ch1;
    if (!data_ready) {
        return std::nullopt;
    }

    bool has_error = (ch == Channel::CH0) ? status->err_chan0 : status->err_chan1;
    if (has_error) {
        log_error(LDC1612, "Error flag set for channel %d (flags: 0x%02X)", static_cast<int>(ch), status->error_flags);
        return std::nullopt;
    }

    return read_channel_data(ch);
}

std::optional<uint32_t> LDC1612::read_channel_data(Channel ch) {
    Register msb_reg = (ch == Channel::CH0) ? Register::DATA0_MSB : Register::DATA1_MSB;
    Register lsb_reg = (ch == Channel::CH0) ? Register::DATA0_LSB : Register::DATA1_LSB;

    uint16_t msb = 0;
    uint16_t lsb = 0;

    // MSB must be read before LSB for data coherency
    if (!read_reg(msb_reg, msb)) {
        return std::nullopt;
    }

    if (!read_reg(lsb_reg, lsb)) {
        return std::nullopt;
    }

    if (msb & 0xF000) {
        log_error(LDC1612, "Per-sample error flags in DATA_MSB: 0x%04X", msb & 0xF000);
        return std::nullopt;
    }

    return parse_channel_data(msb, lsb);
}
