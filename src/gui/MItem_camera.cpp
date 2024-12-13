#include "MItem_camera.hpp"
#include <feature/xbuddy_extension/xbuddy_extension.hpp>

MI_CAM_USB_PWR::MI_CAM_USB_PWR()
    : WI_ICON_SWITCH_OFF_ON_t(buddy::xbuddy_extension().usb_power(), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}

void MI_CAM_USB_PWR::OnChange([[maybe_unused]] size_t old_index) {
    // FIXME: Don't interact with xbuddy_extension directly, but use some common interface, like we have for Chamber API
    buddy::xbuddy_extension().set_usb_power(!old_index);
}

void MI_CAM_USB_PWR::Loop() {
    set_value(buddy::xbuddy_extension().usb_power(), false);
};
