/**
 * @file MItem_MINI.hpp
 */

#pragma once
#include "WindowMenuItems.hpp"
#include "filament_sensor_states.hpp"
#include "i18n.h"
#include <utility_extensions.hpp>

class MI_FILAMENT_SENSOR_STATE final : public IWindowMenuItem {
private:
    static constexpr const char *const label = N_("Filament Sensor");
    FilamentSensorState state = FilamentSensorState::NotInitialized;

protected:
    virtual void printExtension(Rect16 extension_rect, Color color_text, Color color_back, ropfn raster_op) const final;
    virtual void Loop() final;

public:
    MI_FILAMENT_SENSOR_STATE();
};

class MI_MINDA : public WI_SWITCH_0_1_NA_t {
    static constexpr const char *const label = N_("M.I.N.D.A.");
    static state_t get_state();

public:
    MI_MINDA();
    virtual void Loop() override;
    virtual void OnChange([[maybe_unused]] size_t old_index) override {}
};
