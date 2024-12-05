#include "footer_item_fsvalue.hpp"

#include "filament_sensors_handler.hpp"
#include "img_resources.hpp"

FooterItemFSValue::FooterItemFSValue(window_t *parent)
    : FooterIconText_IntVal(parent, &img::spool_16x16, static_makeView, static_readValue) {
}

int FooterItemFSValue::static_readValue() {
    if (IFSensor *sensor = FSensors_instance().sensor(LogicalFilamentSensor::extruder)) {
        return sensor->GetFilteredValue();
    }
    return no_tool_value;
}

string_view_utf8 FooterItemFSValue::static_makeView(int value) {
    // Show --- if no tool is picked
    if (value == no_tool_value) {
        return string_view_utf8::MakeCPUFLASH(reinterpret_cast<const uint8_t *>(no_tool_str));
    }

    static char buff[16];
    snprintf(buff, sizeof(buff), "%d", value);
    return string_view_utf8::MakeRAM((const uint8_t *)buff);
}
