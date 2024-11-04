#include <device/board.h>
#include <option/has_mmu2.h>

#if HAS_MMU2()
    #include <buddy/mmu_port.hpp>

namespace MMU2 {

void power_on() {
    mmu_port::setup_reset_pin();
    mmu_port::power_on();
}

void power_off() {
    mmu_port::power_off();
}

void reset() {
    mmu_port::activate_reset();
    HAL_Delay(5);
    mmu_port::deactivate_reset();
    HAL_Delay(5);
}
} // namespace MMU2
#endif
