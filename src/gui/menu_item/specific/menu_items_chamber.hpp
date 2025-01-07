#pragma once

#include <WindowItemFormatableLabel.hpp>
#include <WindowMenuSpin.hpp>

class MI_CHAMBER_TARGET_TEMP : public WiSpin {
public:
    MI_CHAMBER_TARGET_TEMP(const char *label = N_("Max Chamber Temperature"));

protected:
    virtual void OnClick() override;
    virtual void Loop() override;
};

class MI_CHAMBER_TEMP : public MenuItemAutoUpdatingLabel<float> {
public:
    MI_CHAMBER_TEMP(const char *label = N_("Chamber Temperature"));
};
