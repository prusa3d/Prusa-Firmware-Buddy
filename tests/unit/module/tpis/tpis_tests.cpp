#include <catch2/catch_test_macros.hpp>
#include <i2c/mock.hpp>
#include <tpis/tpis.hpp>
#include <vector>

using namespace i2c::mock;
using MockedTpis = tpis::Tpis<HWImplMock>;

template <typename... Bytes>
constexpr auto make_bytes(Bytes... bytes) {
    return std::vector<std::byte> { static_cast<std::byte>(bytes)... };
}

double calculate_k(const tpis::CalibrationParameters &calibration) {
    double u0 = static_cast<double>(calibration.u0);
    double uout1 = static_cast<double>(calibration.uout1);
    double t_obj1 = static_cast<double>(calibration.t_obj1);
    const auto f = [](double x) { return std::pow(x, 4.2); };
    double k_without_emissivity = (uout1 - u0) / (f(t_obj1 + 273.15) - f(25.0 + 273.15));
    double k = k_without_emissivity * tpis::emissivity;
    return k;
}

tpis::CalibrationParameters get_typical_cal_params() {
    auto cal = tpis::CalibrationParameters {
        .ptat25 = 13500,
        .m = tpis::fixed(172),
        .u0 = 64500,
        .uout1 = 67700,
        .t_obj1 = 100,
        .log2_k = tpis::fixed(0),
    };
    const double k = calculate_k(cal);
    cal.log2_k = tpis::fixed(std::log2(k));
    return cal;
}

tpis::TemperatureReading calculate_temperature_reference(tpis::SensorData measurement, const tpis::CalibrationParameters &calibration) {
    double tp_object = static_cast<double>(measurement.tp_object);
    double tp_ambient = static_cast<double>(measurement.tp_ambient);
    double ptat25 = static_cast<double>(calibration.ptat25);
    double m = static_cast<double>(calibration.m);
    double u0 = static_cast<double>(calibration.u0);

    double t_ambient_k = (25.0 + 273.15) + (tp_ambient - ptat25) * (1 / m);
    const auto f = [](double x) { return std::pow(x, 4.2); };
    double k = calculate_k(calibration);
    const auto F = [](double x) { return std::pow(x, 1.0 / 4.2); };
    double t_obj_k = F((tp_object - u0) / k + f(t_ambient_k));
    return tpis::TemperatureReading {
        .object_temperature_celsius = tpis::fixed(t_obj_k - 273.15),
        .ambient_temperature_celsius = tpis::fixed(t_ambient_k - 273.15),
    };
}

uint32_t get_tp_object_for_temps(double t_obj_c, double t_ambient_c, const tpis::CalibrationParameters &calibration) {
    double u0 = static_cast<double>(calibration.u0);
    double k = calculate_k(calibration);
    const auto f = [](double x) { return std::pow(x, 4.2); };
    uint32_t tp_object = static_cast<uint32_t>(k * (f(t_obj_c + 273.15) - f(t_ambient_c + 273.15)) + u0);
    return tp_object;
}

uint16_t get_tp_ambient_for_temps(double t_obj_c, double t_ambient_c, const tpis::CalibrationParameters &calibration) {
    double ptat25 = static_cast<double>(calibration.ptat25);
    double m = static_cast<double>(calibration.m);
    double t_ambient = t_ambient_c + 273.15;
    double tp_ambient = (t_ambient - (25.0 + 273.15)) * m + ptat25;
    return static_cast<uint16_t>(tp_ambient);
}

tpis::SensorData get_measurement_for_temps(double t_obj_c, double t_ambient_c, const tpis::CalibrationParameters &calibration) {
    return tpis::SensorData {
        .tp_object = get_tp_object_for_temps(t_obj_c, t_ambient_c, calibration),
        .tp_ambient = get_tp_ambient_for_temps(t_obj_c, t_ambient_c, calibration)
    };
}

double get_precision(double ambient, double object) {
    if (ambient > -10.0 && ambient < 80.0 && object > -10.0 && object < 350.0) {
        return 0.075; // for standard temperatures we need precise measurements
    }
    return 0.2; // for safety checks lower precision is good enough
}

TEST_CASE("init", "[tpis]") {
    const auto eeprom_content = make_bytes(
        0x03, // protocol (3)
        0x03, 0xF9, // checksum
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // reserved
        0x02, // lookup (2)
        0x34, 0xBC, // ptat25 (13500)
        0x43, 0x30, // M (17200)
        0x7B, 0xF4, // U0 (31732)
        0x84, 0x3A, // U_out1 (33850)
        0x64, // T_obj1 (100)
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // reserved
        0x00 // slave address
    );

    SECTION("success") {
        MockedTpis tpis;
        HWImplMock &dummy_device = tpis;
        dummy_device.responses = {
            { .success = true, .data = {} }, // general call
            { .success = true, .data = {} }, // enable eeprom reading
            { .success = true, .data = eeprom_content }, // read content
            { .success = true, .data = {} }, // disable eeprom reading
        };
        auto result = tpis.init();
        CHECK(result.has_value());
        std::vector<Call> expected_calls = {
            RawTransmit { .address = 0b0000000, .data = make_bytes(0x04) }, // general call
            Delay { .delay_us = 300 },
            WriteMemory { .address = 0b0001100, .offset = 0x1F, .data = make_bytes(0x80) }, // enable eeprom reading
            ReadMemory { .address = 0b0001100, .offset = 32 }, // read content
            WriteMemory { .address = 0b0001100, .offset = 0x1F, .data = make_bytes(0x00) }, // disable eeprom reading
        };
        CHECK(dummy_device.calls == expected_calls);
        // tpis calibration should be loaded
        auto cal = tpis.get_calibration().value();
        CHECK(cal.ptat25 == 13500);
        CHECK(cal.m == tpis::fixed(172));
        CHECK(cal.u0 == 64500);
        CHECK(cal.uout1 == 67700);
        CHECK(cal.t_obj1 == 100);
        const auto expected_k = 8.27345242232e-8;
        const auto expected_log2_k_with_emissivity = std::log2(expected_k * tpis::emissivity);
        CHECK(std::abs(static_cast<double>(cal.log2_k) - expected_log2_k_with_emissivity) / expected_log2_k_with_emissivity < 1e-4); // relative error < 0.01%
    }

    SECTION("invalid data") {
        auto invalid_checksum = eeprom_content;
        invalid_checksum[1] = std::byte { 0x00 };
        invalid_checksum[2] = std::byte { 0x00 };
        MockedTpis tpis;
        HWImplMock &dummy_device = tpis;
        dummy_device.responses = {
            { .success = true, .data = {} }, // general call
            { .success = true, .data = {} }, // enable eeprom reading
            { .success = true, .data = invalid_checksum }, // read content
            { .success = true, .data = {} }, // disable eeprom reading
        };
        auto result = tpis.init();
        CHECK(!result.has_value());
        CHECK(result.error() == MockedTpis::Error::internal_error);
        CHECK(!tpis.get_calibration().has_value());
        // we still wants to disable reading
        auto &disable = std::get<WriteMemory>(dummy_device.calls.back());
        CHECK(disable == WriteMemory {
                  .address = 0b0001100,
                  .offset = 0x1F,
                  .data = make_bytes(0x00),
              });
    }

    SECTION("i2c error - general call") {
        MockedTpis tpis;
        HWImplMock &dummy_device = tpis;
        dummy_device.responses = {
            { .success = false, .data = {} }, // general call
            { .success = true, .data = {} }, // enable eeprom reading
            { .success = true, .data = eeprom_content }, // read content
            { .success = true, .data = {} }, // disable eeprom reading
        };
        auto result = tpis.init();
        CHECK(!result.has_value());
        CHECK(result.error() == MockedTpis::Error::i2c_error);
        CHECK(!tpis.get_calibration().has_value());
    }

    SECTION("i2c error - enable epprom reading") {
        MockedTpis tpis;
        HWImplMock &dummy_device = tpis;
        dummy_device.responses = {
            { .success = true, .data = {} }, // general call
            { .success = false, .data = {} }, // enable eeprom reading
            { .success = true, .data = eeprom_content }, // read content
            { .success = true, .data = {} }, // disable eeprom reading
        };
        auto result = tpis.init();
        CHECK(!result.has_value());
        CHECK(result.error() == MockedTpis::Error::i2c_error);
        CHECK(!tpis.get_calibration().has_value());
    }

    SECTION("i2c error - read content") {
        MockedTpis tpis;
        HWImplMock &dummy_device = tpis;
        dummy_device.responses = {
            { .success = true, .data = {} }, // general call
            { .success = true, .data = {} }, // enable eeprom reading
            { .success = false, .data = eeprom_content }, // read content
            { .success = true, .data = {} }, // disable eeprom reading
        };
        auto result = tpis.init();
        CHECK(!result.has_value());
        CHECK(result.error() == MockedTpis::Error::i2c_error);
        CHECK(!tpis.get_calibration().has_value());
        // we still wants to disable reading. if it fails... at least I tried
        auto &disable = std::get<WriteMemory>(dummy_device.calls.back());
        CHECK(disable == WriteMemory {
                  .address = 0b0001100,
                  .offset = 0x1F,
                  .data = make_bytes(0x00),
              });
    }

    SECTION("i2c error - disable epprom reading") {
        MockedTpis tpis;
        HWImplMock &dummy_device = tpis;
        dummy_device.responses = {
            { .success = true, .data = {} }, // general call
            { .success = true, .data = {} }, // enable eeprom reading
            { .success = true, .data = eeprom_content }, // read content
            { .success = false, .data = {} }, // disable eeprom reading
        };
        auto result = tpis.init();
        // successfully disabling reading is not critical for correct operation
        CHECK(result.has_value());
        CHECK(tpis.get_calibration().has_value());
        // but still want to try it
        auto &disable = std::get<WriteMemory>(dummy_device.calls.back());
        CHECK(disable == WriteMemory {
                  .address = 0b0001100,
                  .offset = 0x1F,
                  .data = make_bytes(0x00),
              });
    }
}

TEST_CASE("get temps", "[tpis]") {
    SECTION("success") {
        const auto check_for_temps = [](double t_obj_c, double t_ambient_c) {
            const auto calibration = get_typical_cal_params();
            const auto measurement = get_measurement_for_temps(t_obj_c, t_ambient_c, calibration);
            const auto temps_ref = calculate_temperature_reference(measurement, calibration);
            MockedTpis tpis;
            HWImplMock &dummy_device = tpis;
            tpis.set_calibration(calibration); // assume initialized tpis
            const auto sensor_data = make_bytes(
                (measurement.tp_object >> 9) & 0xFF,
                (measurement.tp_object >> 1) & 0xFF,
                ((measurement.tp_object << 7) & 0x80) | ((measurement.tp_ambient >> 8) & 0x7F),
                measurement.tp_ambient & 0xFF);
            dummy_device.responses = { { .success = true, .data = sensor_data } }; // read sensor data
            auto result = tpis.get_temps();
            CHECK(result.has_value());
            // small error is expected since tp_object and tp_ambient are integers
            CHECK(std::abs(static_cast<double>(temps_ref.object_temperature_celsius) - t_obj_c) < 0.2);
            CHECK(std::abs(static_cast<double>(temps_ref.ambient_temperature_celsius) - t_ambient_c) < 0.2);
            const double prec = get_precision(t_ambient_c, t_obj_c);
            CHECK(std::abs(static_cast<double>(result->object_temperature_celsius - temps_ref.object_temperature_celsius)) < prec);
            CHECK(std::abs(static_cast<double>(result->ambient_temperature_celsius - temps_ref.ambient_temperature_celsius)) < prec);
            std::vector<Call> expected_calls = { ReadMemory { .address = 0b0001100, .offset = 0x1 } }; // read sensor data
            CHECK(dummy_device.calls == expected_calls);
        };

        for (double t_ambient_c = -20.0; t_ambient_c < 110.0; t_ambient_c += 1.0) {
            for (double t_obj_c = t_ambient_c - 20.0; t_obj_c < 500.0; t_obj_c += 1.0) {
                check_for_temps(t_obj_c, t_ambient_c);
            }
        }
    }

    SECTION("uninitialized") {
        MockedTpis tpis;
        HWImplMock &dummy_device = tpis;
        auto result = tpis.get_temps();
        CHECK(!result.has_value());
        CHECK(result.error() == MockedTpis::Error::uninitialized);
        CHECK(dummy_device.calls.empty());
    }

    SECTION("i2c error") {
        MockedTpis tpis;
        HWImplMock &dummy_device = tpis;
        tpis.set_calibration(get_typical_cal_params()); // assume initialized tpis
        dummy_device.responses = { { .success = false, .data = make_bytes(0x00, 0x00, 0x00, 0x00) } }; // read sensor data
        auto result = tpis.get_temps();
        CHECK(!result.has_value());
        CHECK(result.error() == MockedTpis::Error::i2c_error);
    }
}
