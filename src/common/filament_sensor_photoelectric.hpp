/**
 * @file filament_sensor_photoelectric.hpp
 * @author Radek Vana
 * @brief photoeletric version of filament sensor
 * @date 2021-02-12
 */

#pragma once

#include "filament_sensor.hpp"

class FSensorPhotoElectric : public IFSensor {
protected:
    virtual void force_set_enabled(bool set) override;
    virtual void cycle() override;

    // just for sensor info visualization of the raw value
    virtual int32_t GetFilteredValue() const override {
        if constexpr (BOARD_IS_XBUDDY()) {
            // Beware - inverted pin logic on XBUDDY
            return state != FilamentSensorState::HasFilament;
        } else {
            return state == FilamentSensorState::HasFilament;
        }
    };

private:
    void set_state(FilamentSensorState set);

    FilamentSensorState last_set_state_target = FilamentSensorState::NotInitialized;

    enum class MeasurePhase {
        p0,
        p1,
    };
    MeasurePhase measure_phase = MeasurePhase::p0;
};
