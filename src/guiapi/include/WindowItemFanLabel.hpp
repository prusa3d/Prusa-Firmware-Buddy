#pragma once
#include "sensor_data_buffer.hpp"
#include "WindowItemFormatableLabel.hpp"
#include <stdint.h>

class WI_FAN_LABEL_t : public WI_FORMATABLE_LABEL_t<std::pair<SensorData::Value, SensorData::Value>> {
private:
    static constexpr auto PWM_MAX { 255 };

public:
    WI_FAN_LABEL_t(string_view_utf8 label, const img::Resource *id_icon, is_enabled_t enabled, is_hidden_t hidden)
        : WI_FORMATABLE_LABEL_t<std::pair<SensorData::Value, SensorData::Value>>(label, id_icon, enabled, hidden, { {}, {} }, [&](char *buffer) {
            if (value.first.attribute.valid && value.second.attribute.valid) {
                const auto pwm = value.first.int_val;
                const auto rpm = value.second.int_val;
                const unsigned int raw_value = (pwm * 100 + PWM_MAX - 1) / PWM_MAX; // ceil div
                const int8_t percent = std::clamp(raw_value, 0u, 100u);
                const char *const format = [&]() {
                    if (rpm) {
                        return N_("%u %% / running");
                    }
                    if (pwm) {
                        return N_("%u %% / stuck");
                    }
                    return N_("%u %% / stopped");
                }();
                snprintf(buffer, GuiDefaults::infoDefaultLen, format, percent);
            } else {
                if (value.first.attribute.valid || value.second.attribute.valid) {
                    strlcpy(buffer, NA, GuiDefaults::infoDefaultLen);
                } else {
                    strlcpy(buffer, NI, GuiDefaults::infoDefaultLen);
                }
            }
        }) {
    }
};
