#pragma once
#include <common/sensor_data.hpp>
#include "WindowItemFormatableLabel.hpp"
#include <stdint.h>

class WI_FAN_LABEL_t : public WI_LAMBDA_LABEL_t {
private:
    static constexpr auto PWM_MAX { 255 };
    int pwm = 0;
    int rpm = 0;

protected:
    virtual void click([[maybe_unused]] IWindowMenu &window_menu) {}

public:
    WI_FAN_LABEL_t(const string_view_utf8 &label, const img::Resource *id_icon, is_enabled_t enabled, is_hidden_t hidden)
        : WI_LAMBDA_LABEL_t(label, id_icon, enabled, hidden, [&](char *buffer) {
            const unsigned int raw_value = (pwm * 100 + PWM_MAX - 1) / PWM_MAX; // ceil div
            const int8_t percent = std::clamp(raw_value, 0u, 100u);
            const char *const format = [&]() -> const char * {
                if (rpm) {
                    return N_("%u %% / %li RPM");
                }
                if (pwm) {
                    return N_("%u %% / stuck");
                }
                return N_("%u %% / stopped");
            }();
            snprintf(buffer, GuiDefaults::infoDefaultLen, format, percent, rpm);
        }) {
    }

    void UpdateValue(int pwm, int rpm) {
        if (this->pwm != pwm || this->rpm != rpm) {
            this->pwm = pwm;
            this->rpm = rpm;
            InValidateExtension();
        }
    }
};
