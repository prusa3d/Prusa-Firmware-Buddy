#pragma once
#include "sensor_data_buffer.hpp"
#include "WindowItemFormatableLabel.hpp"

class WI_FAN_LABEL_t : public WI_FORMATABLE_LABEL_t<std::pair<SensorData::Value, SensorData::Value>> {
public:
    WI_FAN_LABEL_t(string_view_utf8 label, const png::Resource *id_icon, is_enabled_t enabled, is_hidden_t hidden)
        : WI_FORMATABLE_LABEL_t<std::pair<SensorData::Value, SensorData::Value>>(label, id_icon, enabled, hidden, { {}, {} }, [&](char *buffer) {
            if (value.second.attribute.valid || value.first.attribute.valid) {
                int percent = std::clamp((int)((value.first.int_val * 100) / 255), 0, 100);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
                snprintf(buffer, GuiDefaults::infoDefaultLen, "%d %% / %ld rpm", percent, value.second.int_val);
#pragma GCC diagnostic pop
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
