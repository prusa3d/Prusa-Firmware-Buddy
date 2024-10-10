#include "screen_menu_enclosure.hpp"

#include <common/sensor_data.hpp>

#include <device/board.h>
#if XL_ENCLOSURE_SUPPORT()
    #include "xl_enclosure.hpp"
#endif
#include <common/timing.h>

ScreenMenuEnclosure::ScreenMenuEnclosure()
    : detail::ScreenMenuEnclosure(_(label)) {
#if XL_ENCLOSURE_SUPPORT()
    // MI_ENCLOSURE_ENABLE item is Swapped in Control menu with MI_ENCLOSURE. Its visibility depends on xl_enclosure's state (active or not)
    // In this screen MI_ENCLOSURE_ENABLE is always present
    Item<MI_ENCLOSURE_ENABLE>().set_is_hidden(false);
#endif
}

ScreenMenuManualSetting::ScreenMenuManualSetting()
    : detail::ScreenMenuManualSetting(_(label)) {
}
