/**
 * @file fs_event_autolock.hpp
 * @author Radek Vana
 * @brief ctor locks events (autoload and M600 injection), dtor unlocks it
 * normally it is handled automaticaly but in some cases manual call is needed
 * @date 2021-02-14
 */

#pragma once

#include "filament_sensor.hpp"

class FS_EventAutolock {
public:
    inline FS_EventAutolock() { FS_instance().IncEvLock(); }
    inline ~FS_EventAutolock() { FS_instance().DecEvLock(); }
};
