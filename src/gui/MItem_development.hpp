/// @file
#pragma once

#include "WindowMenuItems.hpp"
#include <option/development_items.h>
#include <option/has_attachable_accelerometer.h>

static_assert(DEVELOPMENT_ITEMS());

class MI_DRY_RUN final : public WI_ICON_SWITCH_OFF_ON_t {
public:
    MI_DRY_RUN();

protected:
    virtual void OnChange(size_t) override;
};

class MI_TRIGGER_BANK_MIGRATION final : public IWindowMenuItem {
public:
    MI_TRIGGER_BANK_MIGRATION();

protected:
    void click(IWindowMenu &) override;
};

class MI_WAVETABLE_XYZ final : public WI_ICON_SWITCH_OFF_ON_t {
public:
    MI_WAVETABLE_XYZ();

protected:
    virtual void OnChange(size_t) override;
};

#if HAS_ATTACHABLE_ACCELEROMETER()
/// Developer-only manual probe: detect an accelerometer connected during uptime
/// Use carefully as (re/dis)connecting should NOT be done during uptime.
class MI_CHECK_ACCELEROMETER final : public IWindowMenuItem {
public:
    MI_CHECK_ACCELEROMETER();

protected:
    void click(IWindowMenu &window_menu) override;
};
#endif
