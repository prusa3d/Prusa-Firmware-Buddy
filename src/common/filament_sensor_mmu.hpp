/**
 * @file filament_sensor_mmu.hpp
 * @brief clas representing filament sensor of MMU
 */

#pragma once

#include "filament_sensor.hpp"

class FSensorMMU : public IFSensor {
protected:
    virtual void record_state() override; // record metrics
    virtual void cycle() override;
    virtual void enable() override;
    virtual void disable() override;
};
