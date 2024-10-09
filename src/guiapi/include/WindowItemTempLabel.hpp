#pragma once
#include <common/sensor_data.hpp>
#include "WindowItemFormatableLabel.hpp"

class WI_TEMP_LABEL_t : public WI_FORMATABLE_LABEL_t<float> {
public:
    WI_TEMP_LABEL_t(const string_view_utf8 &label, const img::Resource *id_icon, is_enabled_t enabled, is_hidden_t hidden)
        : WI_FORMATABLE_LABEL_t<float>(label, id_icon, enabled, hidden, {}, [&](const std::span<char> &buffer) {
            snprintf(buffer.data(), buffer.size(), "%.1f\xC2\xB0\x43", (double)value); // \xB0\x43 == degree Celsius
        }) {}
};
