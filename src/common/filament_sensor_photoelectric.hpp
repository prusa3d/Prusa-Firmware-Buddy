/**
 * @file filament_sensor_photoelectric.hpp
 * @author Radek Vana
 * @brief photoeletric version of filament sensor
 * @date 2021-02-12
 */

#pragma once

#include "filament_sensor.hpp"

class FSensorPhotoElectric : public FSensor {
protected:
    enum class Cycle { no0,
        no1 };
    Cycle measure_cycle = Cycle::no0;

    virtual void set_state(fsensor_t st) override;

    virtual void enable() override;
    virtual void disable() override;
    virtual void cycle() override;
    void cycle0();
    void cycle1();

    virtual void record_state() override; // record metrics

public:
    fsensor_t WaitInitialized();

    FSensorPhotoElectric();
};
