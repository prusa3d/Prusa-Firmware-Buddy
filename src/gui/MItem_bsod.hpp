#pragma once
#include "WindowMenuItems.hpp"
#include "i18n.h"
#include "config_features.h"

class MI_TRIGGER_BSOD : public WI_LABEL_t {
public:
    MI_TRIGGER_BSOD();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};
