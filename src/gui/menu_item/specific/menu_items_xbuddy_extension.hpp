#pragma once

#include <WindowMenuSpin.hpp>

/// Automatically hidden if the extboard is disabled
class MI_XBUDDY_EXTENSION_LIGHTS : public WiSpin {
public:
    MI_XBUDDY_EXTENSION_LIGHTS();
    virtual void OnClick() override;
};

/// Manual control for chamber fans
class MI_XBUDDY_EXTENSION_COOLING_FANS : public WiSpin {
public:
    MI_XBUDDY_EXTENSION_COOLING_FANS();
    virtual void OnClick() override;
};
