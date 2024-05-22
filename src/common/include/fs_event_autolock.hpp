/**
 * @file fs_event_autolock.hpp
 * @author Radek Vana
 * @brief ctor locks events (autoload and M600 injection), dtor unlocks it
 * normally it is handled automaticaly but in some cases manual call is needed
 * @date 2021-02-14
 */

#pragma once

#include "filament_sensors_handler.hpp"

class FS_EventAutolock {
public:
    FS_EventAutolock(const FS_EventAutolock &other) = delete;
    FS_EventAutolock(FS_EventAutolock &&other) = delete;
    inline FS_EventAutolock() { FSensors_instance().IncEvLock(); }
    inline ~FS_EventAutolock() { FSensors_instance().DecEvLock(); }
};
