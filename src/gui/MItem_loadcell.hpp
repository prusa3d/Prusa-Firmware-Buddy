/**
 * @file MItem_loadcell.hpp
 * @author Radek Vana
 * @brief loadcell menu items
 * @date 2021-10-03
 */
#pragma once
#include "WindowMenuItems.hpp"
#include "i18n.h"

class MI_TEST_LOADCELL : public WI_LABEL_t {
    static constexpr const char *const label = N_("Test Loadcell");

public:
    MI_TEST_LOADCELL();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};
