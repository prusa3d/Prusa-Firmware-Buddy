#pragma once
#include <common/sensor_data.hpp>
#include "WindowItemFormatableLabel.hpp"
#include <stdint.h>

struct FanPWMAndRPM {
    uint8_t pwm = 0;
    uint16_t rpm = 0;

    bool operator==(const FanPWMAndRPM &) const = default;
};

class WI_FAN_LABEL_t : public MenuItemAutoUpdatingLabel<FanPWMAndRPM> {
private:
    static constexpr auto PWM_MAX { 255 };

public:
    WI_FAN_LABEL_t(const string_view_utf8 &label, const GetterFunction &getter)
        : MenuItemAutoUpdatingLabel(
            label, [this](auto &buf) { print_value(buf); }, getter) {
    }

private:
    void print_value(const std::span<char> &buffer) {
        const auto val = value();

        const unsigned int raw_value = (val.pwm * 100 + PWM_MAX - 1) / PWM_MAX; // ceil div
        const int8_t percent = std::clamp(raw_value, 0u, 100u);

        const char *const format = //
            val.rpm   ? N_("%u %% / %li RPM")
            : val.pwm ? N_("%u %% / stuck")
                      : N_("%u %% / stopped");

        snprintf(buffer.data(), buffer.size(), format, percent, val.rpm);
    }
};
