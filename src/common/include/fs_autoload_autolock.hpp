#pragma once

#include "filament_sensor_api.hpp"

// ctor lock autoload and dtor unlocks autoload
class FS_AutoloadAutolock {
public:
    FS_AutoloadAutolock(const FS_AutoloadAutolock &other) = delete;
    FS_AutoloadAutolock(FS_AutoloadAutolock &&other) = delete;
    inline FS_AutoloadAutolock() { FSensors_instance().IncAutoloadLock(); }
    inline ~FS_AutoloadAutolock() { FSensors_instance().DecAutoloadLock(); }
};
