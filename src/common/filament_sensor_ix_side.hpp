#pragma once

#include "filament_sensor.hpp"

/// Side filament sensor for iX, mounted on the back door right before the spool.
/// Uses ports formerly intended for touchscreen I2C.
/// PC9: State (high = filament detected, low = filament not detected)
/// PA8: Sensor connected detection (high = not connected [thanks to the pullup on the Buddy board], low = connected)
/// More info in BFW-4746
class FSensor_iXSide : public IFSensor {
protected:
    virtual void cycle() override;
};
