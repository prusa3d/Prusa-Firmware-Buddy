/**
 * @file screen_menu_no_tools.hpp
 */
#pragma once

#include "WindowItemTempLabel.hpp"
#include "i18n.h"

class MI_INFO_NOZZLE_TEMP : public WI_TEMP_LABEL_t {
    static constexpr const char *const label = N_("Nozzle Temperature");

public:
    MI_INFO_NOZZLE_TEMP();
};
