#pragma once
#include "sensor_data_buffer.hpp"
#include "WindowItemFormatableLabel.hpp"
#include <stdint.h>

class WI_FAN_LABEL_t : public WI_FORMATABLE_LABEL_t<std::pair<SensorData::Value, SensorData::Value>> {
private:
    static constexpr auto PWM_MAX { 255 };

public:
    WI_FAN_LABEL_t(const string_view_utf8 &label, const img::Resource *id_icon, is_enabled_t enabled, is_hidden_t hidden)
        : WI_FORMATABLE_LABEL_t<std::pair<SensorData::Value, SensorData::Value>>(label, id_icon, enabled, hidden, { {}, {} }, [&](char *buffer) {
            if (value.first.is_valid() && value.second.is_valid()) {
                const auto pwm = value.first.get_int();
                const auto rpm = value.second.get_int();
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
            } else {
                if (value.first.is_enabled() || value.second.is_valid()) {
                    strlcpy(buffer, NA, GuiDefaults::infoDefaultLen);
                } else {
                    strlcpy(buffer, NI, GuiDefaults::infoDefaultLen);
                }
            }
        }) {
    }
};
