#include "filament_sensor_api.hpp"
#include "../../lib/Marlin/Marlin/src/feature/prusa_MMU2/mmu2_fsensor.h"

namespace mmu {

FilamentState WhereIsFilament() {
    return FSensors_instance().WhereIsFilament();
}

BlockRunoutRAII::BlockRunoutRAII() {
    FSensors_instance().IncEvLock();
}

BlockRunoutRAII::~BlockRunoutRAII() {
    FSensors_instance().DecEvLock();
}

} // namespace mmu
