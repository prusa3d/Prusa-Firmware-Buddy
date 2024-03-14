/**
 * @file MItem_love_board.hpp
 */
#pragma once

#include "WindowMenuItems.hpp"
#include "WindowItemTempLabel.hpp"
#include "i18n.h"
#include "otp_types.hpp"

class MI_INFO_SERIAL_NUM_LOVEBOARD : public WiInfo<28> {
    static constexpr const char *const label = N_("Love Board");

public:
    MI_INFO_SERIAL_NUM_LOVEBOARD();

    using Buff = std::array<char, sizeof(LoveBoardEeprom::datamatrix) + 1 + 3 + 1>;
    static Buff to_array();
};

class MI_INFO_HEATBREAK_TEMP : public WI_TEMP_LABEL_t {
    static constexpr const char *const label = N_("Heatbreak Temp");

public:
    MI_INFO_HEATBREAK_TEMP();
};

class MI_LOVEBOARD_SINGLE_ERR : public WiSpinInt {
    constexpr static const char *const label = "Loveboard Single e.";

public:
    MI_LOVEBOARD_SINGLE_ERR();
};

class MI_LOVEBOARD_REPEATED_ERR : public WiSpinInt {
    constexpr static const char *const label = "Loveboard Repeated e.";

public:
    MI_LOVEBOARD_REPEATED_ERR();
};

class MI_LOVEBOARD_CYCLIC_ERR : public WiSpinInt {
    constexpr static const char *const label = "Loveboard Cyclic e.";

public:
    MI_LOVEBOARD_CYCLIC_ERR();
};

class MI_LOVEBOARD_RETRIED : public WiSpinInt {
    constexpr static const char *const label = "Loveboard Retried";

public:
    MI_LOVEBOARD_RETRIED();
};

class MI_LOVEBOARD_STATUS : public WI_INFO_DEV_t {
    constexpr static const char *const label = "Loveboard Status";

public:
    MI_LOVEBOARD_STATUS();
};
