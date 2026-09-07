/**
 * @file MItem_touch.hpp
 * @brief Touch related menu items
 */
#pragma once
#include "WindowMenuItems.hpp"

class MI_ENABLE_TOUCH : public WI_ICON_SWITCH_OFF_ON_t {
    constexpr static const char *const label = N_("Touch");

public:
    MI_ENABLE_TOUCH();

    virtual void OnChange(size_t old_index) override;
};

class TOUCH_SIG_WORKAROUND : public WI_ICON_SWITCH_OFF_ON_t {
    constexpr static const char *const label = N_("Touch Sig Workaround");

public:
    TOUCH_SIG_WORKAROUND();

    virtual void OnChange(size_t old_index) override;
};
