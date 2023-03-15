#pragma once
#include "sensor_data_buffer.hpp"
#include "WindowItemFormatableLabel.hpp"

class WI_TEMP_LABEL_t : public WI_FORMATABLE_LABEL_t<SensorData::Value> {
public:
    WI_TEMP_LABEL_t(string_view_utf8 label, const png::Resource *id_icon, is_enabled_t enabled, is_hidden_t hidden)
        : WI_FORMATABLE_LABEL_t<SensorData::Value>(label, id_icon, enabled, hidden, {}, [&](char *buffer) {
            if (value.attribute.valid) {
                snprintf(buffer, GuiDefaults::infoDefaultLen, "%.1f\177C", (double)value.float_val);
            } else {
                if (value.attribute.valid) {
                    strlcpy(buffer, NA, GuiDefaults::infoDefaultLen);
                } else {
                    strlcpy(buffer, NI, GuiDefaults::infoDefaultLen);
                }
            }
        }) {}
};
