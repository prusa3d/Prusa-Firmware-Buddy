#include "screen_menu_enclosure.hpp"
#include "../common/sensor_data_buffer.hpp"

#include <device/board.h>
#if XL_ENCLOSURE_SUPPORT()
    #include "xl_enclosure.hpp"
#endif

ScreenMenuEnclosure::ScreenMenuEnclosure()
    : detail::ScreenMenuEnclosure(_(label))
    , last_ticks_s(ticks_s()) {
#if XL_ENCLOSURE_SUPPORT()
    // MI_ENCLOSURE_ENABLE item is Swapped in Control menu with MI_ENCLOSURE. Its visibility depends on xl_enclosure's state (active or not)
    // In this screen MI_ENCLOSURE_ENABLE is always present
    Show<MI_ENCLOSURE_ENABLE>();
#endif
}

void ScreenMenuEnclosure::windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param) {
#if XL_ENCLOSURE_SUPPORT()
    if (event == GUI_event_t::LOOP) {
        if (ticks_s() - last_ticks_s >= loop_delay_s) {
            last_ticks_s = ticks_s();
            Item<MI_ENCLOSURE_TEMP>().UpdateValue(xl_enclosure.getEnclosureTemperature());
        }

    } else if (event == GUI_event_t::CLICK) {
        last_ticks_s = ticks_s();
    }
#endif
    SuperWindowEvent(sender, event, param);
}

ScreenMenuManualSetting::ScreenMenuManualSetting()
    : detail::ScreenMenuManualSetting(_(label)) {
}
