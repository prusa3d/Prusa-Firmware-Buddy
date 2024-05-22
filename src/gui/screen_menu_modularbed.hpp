/**
 * @file
 */

#pragma once
#include "WindowMenuItems.hpp"
#include "WindowItemTempLabel.hpp"

class MI_HEAT_ENTIRE_BED : public WI_ICON_SWITCH_OFF_ON_t {
    constexpr static const char *const label = N_("Heat Entire Bed");

public:
    MI_HEAT_ENTIRE_BED();

protected:
    virtual void OnChange(size_t old_index) override;
};

class MI_INFO_MODULAR_BED_MCU_TEMPERATURE : public WI_TEMP_LABEL_t {
    static constexpr const char *const label = N_("MBed MCU Temp");

public:
    MI_INFO_MODULAR_BED_MCU_TEMPERATURE();
};
