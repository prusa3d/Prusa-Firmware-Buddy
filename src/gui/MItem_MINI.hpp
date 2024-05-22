/**
 * @file MItem_MINI.hpp
 */

#pragma once
#include "WindowMenuItems.hpp"
#include "i18n.h"
#include <utility_extensions.hpp>

class MI_FILAMENT_SENSOR_STATE : public WI_SWITCH_0_1_NA_t {
    static constexpr const char *const label = N_("Filament Sensor");
    static state_t get_state();

public:
    MI_FILAMENT_SENSOR_STATE();
    virtual void Loop() override;
    virtual void OnChange([[maybe_unused]] size_t old_index) override {}
};

class MI_MINDA : public WI_SWITCH_0_1_NA_t {
    static constexpr const char *const label = N_("M.I.N.D.A.");
    static state_t get_state();

public:
    MI_MINDA();
    virtual void Loop() override;
    virtual void OnChange([[maybe_unused]] size_t old_index) override {}
};
