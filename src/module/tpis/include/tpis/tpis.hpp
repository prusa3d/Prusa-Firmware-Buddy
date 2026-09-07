/// @file
/// Code for TPiS infrared temperature sensor
/// https://www.excelitas.com/assets/product/document/calipile-tpis-1t-1086-l55-datasheet.pdf?file=25046.pdf
#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <expected>
#include <optional>
#include <span>

#include <bsod/bsod.h>
#include <fpm/fixed.hpp>
#include <fpm/math.hpp>
#include <i2c/base.hpp>

namespace tpis {

inline constexpr size_t fraction_bits = 15;
using fixed = fpm::fixed<int32_t, int64_t, fraction_bits>;
inline constexpr size_t integral_bits = sizeof(fixed) * 8 - fraction_bits;

inline constexpr float emissivity = 0.48f;

struct SensorData {
    uint32_t tp_object = 0;
    uint16_t tp_ambient = 0;
};

struct CalibrationParameters {
    uint16_t ptat25 = 0;
    fixed m { 0 };
    uint32_t u0 = 0;
    uint32_t uout1 = 0;
    uint8_t t_obj1 = 0;
    fixed log2_k = fixed(0);
};

struct TemperatureReading {
    fixed object_temperature_celsius;
    fixed ambient_temperature_celsius;
};

template <i2c::I2cBus HWImpl>
class Tpis : public HWImpl {
public:
    enum class Error {
        i2c_error,
        internal_error,
        uninitialized
    };

    template <typename T>
    using Result = std::expected<T, Error>;

    [[nodiscard]] Result<void> init() {
        if (auto result = do_general_call(); !result.has_value()) {
            return result;
        }
        if (auto result = enable_eeprom_reading(); !result.has_value()) {
            return result;
        }
        auto cal = read_eeprom_calibration();
        auto _ = disable_eeprom_reading();
        if (!cal.has_value()) {
            return std::unexpected(cal.error());
        }
        calibration = cal.value();
        return {};
    }

    [[nodiscard]] Result<TemperatureReading> get_temps() {
        if (!calibration.has_value()) {
            return std::unexpected(Error::uninitialized);
        }
        const auto measurement = read_sensor_data();
        if (!measurement.has_value()) {
            return std::unexpected(measurement.error());
        }
        return calculate_temps(measurement.value());
    }

#if UNITTESTS
    const std::optional<CalibrationParameters> &get_calibration() {
        return calibration;
    }

    void set_calibration(const std::optional<CalibrationParameters> &cal) {
        calibration = cal;
    }
#endif

private:
    static constexpr float degC0asKf = 273.15f;
    static constexpr fixed degC0asK = fixed(degC0asKf);
    static constexpr float degC25asKf = 25.f + degC0asKf;
    static constexpr fixed degC25asK = fixed(degC25asKf);
    static constexpr float f_exp_f = 4.2f;
    static constexpr fixed f_exp = fixed(f_exp_f);
    static constexpr float F_exp_f = 1 / f_exp_f;
    static constexpr fixed F_exp = fixed(F_exp_f);

    static constexpr i2c::Address address = 0x0C;
    std::optional<CalibrationParameters> calibration {};

    [[nodiscard]] Result<void> do_general_call() {
        static constexpr std::array<std::byte, 1> reload_command = { std::byte { 0x04 } };
        if (!this->raw_transmit(0x00, reload_command)) {
            return std::unexpected(Error::i2c_error);
        }
        static constexpr uint32_t reloading_time_us = 300;
        this->delay_us(reloading_time_us);
        return {};
    }

    [[nodiscard]] Result<void> enable_eeprom_reading() {
        static constexpr std::byte ecr_enable = std::byte { 0x80 };
        return set_ecr(ecr_enable);
    }

    [[nodiscard]] Result<void> disable_eeprom_reading() {
        static constexpr std::byte ecr_disable = std::byte { 0x00 };
        return set_ecr(ecr_disable);
    }

    [[nodiscard]] Result<void> set_ecr(std::byte value) {
        static constexpr uint8_t ecr_address = 0x1F;
        const std::array<std::byte, 1> data = { value };
        if (!this->write_memory(address, ecr_address, data)) {
            return std::unexpected(Error::i2c_error);
        }
        return {};
    }

    [[nodiscard]] Result<CalibrationParameters> read_eeprom_calibration() {
        std::array<std::byte, 32> raw {};
        if (!this->read_memory(address, 32, raw)) {
            return std::unexpected(Error::i2c_error);
        }
        auto calibration = decode_calibration_parameters(raw);
        if (!calibration.has_value()) {
            return std::unexpected(Error::internal_error);
        }
        return calibration.value();
    }

    [[nodiscard]] Result<SensorData> read_sensor_data() {
        std::array<std::byte, 4> raw {};
        if (!this->read_memory(address, 0x01, raw)) {
            return std::unexpected(Error::i2c_error);
        }
        return decode_sensor_data(raw);
    }

    [[nodiscard]] static SensorData decode_sensor_data(std::span<const std::byte, 4> raw_data) {
        uint32_t tp_object = (static_cast<uint32_t>(raw_data[0]) << 8 | static_cast<uint32_t>(raw_data[1])) << 1 | static_cast<uint32_t>(raw_data[2] >> 7);
        uint16_t tp_ambient = (static_cast<uint16_t>(raw_data[2] & std::byte { 0x7f }) << 8) | static_cast<uint16_t>(raw_data[3]);
        return SensorData { .tp_object = tp_object, .tp_ambient = tp_ambient };
    }

    [[nodiscard]] static std::optional<CalibrationParameters> decode_calibration_parameters(std::span<const std::byte, 32> raw_data) {
        const uint8_t protocol = static_cast<uint8_t>(raw_data[0]);
        if (protocol != 0x3) {
            return std::nullopt;
        }

        if (!validate_checksum(raw_data)) {
            return std::nullopt;
        }

        uint8_t lookup = static_cast<uint8_t>(raw_data[9]);
        if (lookup != 2) {
            return std::nullopt;
        }

        const uint16_t ptat25 = static_cast<uint16_t>(raw_data[10]) << 8 | static_cast<uint16_t>(raw_data[11]);
        const uint16_t raw_m_reg = static_cast<uint16_t>(raw_data[12]) << 8 | static_cast<uint16_t>(raw_data[13]);
        const fixed m = fixed(raw_m_reg) / 100;
        const auto raw_u0_reg = static_cast<uint16_t>(raw_data[14]) << 8 | static_cast<uint16_t>(raw_data[15]);
        const uint32_t u0 = raw_u0_reg + 32768;
        const auto raw_uout1_reg = static_cast<uint16_t>(raw_data[16]) << 8 | static_cast<uint16_t>(raw_data[17]);
        const uint32_t uout1 = raw_uout1_reg * 2;
        const uint8_t t_obj1 = static_cast<uint8_t>(raw_data[18]);

        const auto u_div = static_cast<int32_t>(uout1) - static_cast<int32_t>(u0);
        // NOTE: Expensive float op, but OK since it is ideally only done once at init
        const auto f = [](float x) { return std::pow(x, f_exp_f); };
        const float k_f = static_cast<float>(u_div) / (f(t_obj1 + degC0asKf) - f(degC25asKf));
        const float log2_k_f = std::log2(k_f * tpis::emissivity);
        const fixed log2_k = fixed(log2_k_f);

        return CalibrationParameters {
            .ptat25 = ptat25,
            .m = m,
            .u0 = u0,
            .uout1 = uout1,
            .t_obj1 = t_obj1,
            .log2_k = log2_k
        };
    }

    [[nodiscard]] static bool validate_checksum(std::span<const std::byte, 32> data) {
        auto checksum = static_cast<uint16_t>(data[0]);
        const uint16_t expected_checksum = (static_cast<uint16_t>(data[1]) << 8) | static_cast<uint16_t>(data[2]);
        for (size_t i = 3; i < data.size(); ++i) {
            checksum += static_cast<uint8_t>(data[i]);
        }
        return checksum == expected_checksum;
    }

    [[nodiscard]] TemperatureReading calculate_temps(SensorData measurement) {
        static_assert(fraction_bits >= 15);
        static_assert(integral_bits >= 17);
        contract_assert(calibration.has_value());
        const fixed t_ambient_k = degC25asK + fixed(static_cast<int32_t>(measurement.tp_ambient) - static_cast<int32_t>(calibration->ptat25)) / calibration->m;
        const fixed tp_relative = fixed(static_cast<int32_t>(measurement.tp_object) - static_cast<int32_t>(calibration->u0));
        constexpr fixed eps = fixed(1) / 1000;
        // https://en.wikipedia.org/wiki/LogSumExp#log-sum-exp_trick_for_log-domain_calculations
        const fixed lse_x = fpm::log2(fpm::abs(tp_relative) + eps) - calibration->log2_k;
        const fixed lse_y = f_exp * fpm::log2(t_ambient_k);
        const auto fixed_max = [](fixed x, fixed y) { return x > y ? x : y; };
        const auto checked_exp2 = [](fixed x) {
            return x < fixed(1 - fraction_bits) ? fixed(0) : fpm::exp2(x); // fpm::exp2 is broken on underflow
        };
        const fixed lse_exp = checked_exp2(-fpm::abs(lse_x - lse_y));
        const fixed lse_sum = tp_relative >= fixed(0) ? 1 + lse_exp : 1 - lse_exp;
        if (lse_sum <= fixed(0)) [[unlikely]] {
            abort();
        }
        const fixed lse = fixed_max(lse_x, lse_y) + fpm::log2(lse_sum);
        const fixed t_obj_k = fpm::exp2(F_exp * lse);
        const fixed object_temperature_celsius = t_obj_k - degC0asK;
        const fixed ambient_temperature_celsius = t_ambient_k - degC0asK;
        return TemperatureReading { object_temperature_celsius, ambient_temperature_celsius };
    }
};

} // namespace tpis
