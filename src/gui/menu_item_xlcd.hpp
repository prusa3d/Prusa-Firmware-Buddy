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

    using Buff = std::array<char, sizeof(XlcdEeprom::datamatrix) + 1 + 3 + 1>;
    static Buff to_array();
};

class MI_XLCD_SINGLE_ERR : public WiSpinInt {
    constexpr static const char *const label = "XLCD Single e.";

public:
    MI_XLCD_SINGLE_ERR();
};

class MI_XLCD_REPEATED_ERR : public WiSpinInt {
    constexpr static const char *const label = "XLCD Repeated e.";

public:
    MI_XLCD_REPEATED_ERR();
};

class MI_XLCD_CYCLIC_ERR : public WiSpinInt {
    constexpr static const char *const label = "XLCD Cyclic e.";

public:
    MI_XLCD_CYCLIC_ERR();
};

class MI_XLCD_RETRIED : public WiSpinInt {
    constexpr static const char *const label = "XLCD Retried";

public:
    MI_XLCD_RETRIED();
};

class MI_XLCD_STATUS : public WI_INFO_DEV_t {
    constexpr static const char *const label = "XLCD Status";

public:
    MI_XLCD_STATUS();
};
