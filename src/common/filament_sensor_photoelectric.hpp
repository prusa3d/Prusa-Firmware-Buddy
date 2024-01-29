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
    virtual void enable() override;
    virtual void disable() override;
    virtual void cycle() override;

private:
    void set_state(FilamentSensorState set);

    FilamentSensorState last_set_state_target = FilamentSensorState::NotInitialized;

    enum class MeasurePhase {
        p0,
        p1,
    };
    MeasurePhase measure_phase = MeasurePhase::p0;

public:
    FSensorPhotoElectric();
};
