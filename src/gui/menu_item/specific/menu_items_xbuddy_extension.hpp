#pragma once

#include <WindowMenuSpin.hpp>

/// Automatically hidden if the extboard is disabled
class MI_XBUDDY_EXTENSION_LIGHTS : public WiSpin {
public:
    MI_XBUDDY_EXTENSION_LIGHTS();
    virtual void OnClick() override;
};
