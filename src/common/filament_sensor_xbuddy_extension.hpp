#pragma once

#include "filament_sensor.hpp"

class FSensorXBuddyExtension : public IFSensor {
protected:
    virtual void cycle() override;
    virtual int32_t GetFilteredValue() const override;

private:
    FilamentSensorState interpret_state() const;
};
