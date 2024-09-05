/**
 * @file menu_item_xlcd.hpp
 */
#pragma once

#include "WindowMenuItems.hpp"
#include "otp_types.hpp"
#include "i18n.h"

class MI_INFO_SERIAL_NUM_XLCD : public WiInfo<28> {
    static constexpr const char *const label = N_("xLCD");

public:
    MI_INFO_SERIAL_NUM_XLCD();
};
