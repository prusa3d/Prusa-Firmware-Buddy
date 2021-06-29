/**
 * @file filament_sensor_photoeletric.hpp
 * @author Radek Vana
 * @brief photoeletric version of filament sensor
 * @date 2021-02-12
 */

#pragma once

#include "filament_sensor.hpp"

class FSensorPhotoElectric : public FSensor {
    uint8_t meas_cycle;
    void cycle0();
    void cycle1();

protected:
    virtual void enable() override;
    virtual void disable() override;
    virtual void cycle() override;

public:
    FSensorPhotoElectric()
        : meas_cycle(0) {}
};
