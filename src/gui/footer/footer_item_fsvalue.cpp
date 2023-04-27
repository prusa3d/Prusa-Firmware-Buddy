#include "footer_item_fsvalue.hpp"

#include "filament_sensors_handler.hpp"
#include "display_helper.h" // font_meas_text
#include "png_resources.hpp"

FooterItemFSValue::FooterItemFSValue(window_t *parent)
    : AddSuperWindow<FooterIconText_IntVal>(parent, &png::spool_16x16, static_makeView, static_readValue) {
}

int FooterItemFSValue::static_readValue() {
    IFSensor *sensor = get_active_printer_sensor();
    if (sensor) {
        return sensor->GetFilteredValue();
    }
    return std::numeric_limits<int>::min();
}

string_view_utf8 FooterItemFSValue::static_makeView(int value) {
    static char buff[16];
    if (value == std::numeric_limits<int>::min()) {
        strncpy(buff, "---", sizeof(buff));
    } else {
        snprintf(buff, sizeof(buff), "%d", value);
    }
    return string_view_utf8::MakeRAM((const uint8_t *)buff);
}
