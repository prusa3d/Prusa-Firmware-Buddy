#pragma once

#include <WindowMenuItems.hpp>

class MI_CAM_USB_PWR : public WI_ICON_SWITCH_OFF_ON_t {
    constexpr static const char *const label = N_("Camera");

public:
    MI_CAM_USB_PWR();
    void OnChange(size_t old_index) final;
    void Loop() final;
};
