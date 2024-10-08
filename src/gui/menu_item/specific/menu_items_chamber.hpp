#pragma once

#include <WindowMenuSpin.hpp>

class MI_CHAMBER_TARGET_TEMP : public WiSpin {
public:
    MI_CHAMBER_TARGET_TEMP(const char *label = nullptr);
    virtual void OnClick() override;
};
