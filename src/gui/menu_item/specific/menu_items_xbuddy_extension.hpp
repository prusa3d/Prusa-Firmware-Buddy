#pragma once

#include <WindowItemFanLabel.hpp>
#include <WindowMenuSpin.hpp>
#include <WindowMenuItems.hpp>

/// Automatically hidden if the extboard is disabled
class MI_XBUDDY_EXTENSION_LIGHTS : public WiSpin {
public:
    MI_XBUDDY_EXTENSION_LIGHTS();
    virtual void OnClick() override;
};

/// Manual control for chamber fans
/// Automatically hidden if the extboard is disabled
class MI_XBUDDY_EXTENSION_COOLING_FANS : public WiSpin {
public:
    MI_XBUDDY_EXTENSION_COOLING_FANS();
    virtual void OnClick() override;
};

/// Manual control for chamber filtration fan
class MI_XBE_FILTRATION_FAN : public WiSpin {
public:
    MI_XBE_FILTRATION_FAN();
    virtual void OnClick() override;
};

/// PWM/RPM info for fan1
/// Automatically hidden if the extboard is disabled
class MI_INFO_XBUDDY_EXTENSION_FAN1 : public WI_FAN_LABEL_t {
public:
    MI_INFO_XBUDDY_EXTENSION_FAN1();
};

/// PWM/RPM info for fan2
/// Automatically hidden if the extboard is disabled
class MI_INFO_XBUDDY_EXTENSION_FAN2 : public WI_FAN_LABEL_t {
public:
    MI_INFO_XBUDDY_EXTENSION_FAN2();
};

/// PWM/RPM info for fan3
/// Automatically hidden if the extboard is disabled
class MI_INFO_XBUDDY_EXTENSION_FAN3 : public WI_FAN_LABEL_t {
public:
    MI_INFO_XBUDDY_EXTENSION_FAN3();
};

/// USB power control (for camera)
class MI_CAM_USB_PWR : public WI_ICON_SWITCH_OFF_ON_t {
public:
    MI_CAM_USB_PWR();
    void OnChange(size_t old_index) final;
    void Loop() final;
};
