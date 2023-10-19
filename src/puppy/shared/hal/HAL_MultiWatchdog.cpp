#include "HAL_MultiWatchdog.hpp"
#include <stm32g0xx_ll_bus.h>
#include <stm32g0xx_ll_system.h>
#include "cmsis_os.h"

namespace hal {

MultiWatchdog *MultiWatchdog::list = nullptr;
bool MultiWatchdog::initialized = false;
IWDG_HandleTypeDef MultiWatchdog::hiwdg = {
    .Instance = IWDG,
    .Init = {
        .Prescaler = 0x6, // Slowest, approx 32 kHz / 256 = 125 Hz
        .Reload = 0xfff, // Maximal, approx 2^12 / (~32 kHz / 256) = 32.768 s
        .Window = 0xfff, // Window disabled
    },
};

/**
 * @brief Create an instance of a watchdog and add it to the global list.
 */
MultiWatchdog::MultiWatchdog() {
    // Add itself on top of list
    __disable_irq();
    next = list;
    list = this;
    __enable_irq();
}

/**
 * @brief Initialize hardware watchdog.
 * This needs to be done only once for all instances.
 */
void MultiWatchdog::init() {
    __disable_irq();
    if (initialized) {
        __enable_irq();
        return;
    }

    // Freeze watchdog on debugger break
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_DBGMCU);
    LL_DBGMCU_APB1_GRP1_FreezePeriph(LL_DBGMCU_APB1_GRP1_IWDG_STOP);
    LL_APB1_GRP1_DisableClock(LL_APB1_GRP1_PERIPH_DBGMCU);

    // Init watchdog
    HAL_IWDG_Init(&hiwdg);

    initialized = true;
    __enable_irq();
}

/**
 * @brief Refresh one instance of the watchdog.
 * @param hardware true to reset hardware, false to only mark (another instance must reset the hardware)
 */
void MultiWatchdog::kick(bool hardware) {
    mark = 0xff; // Mark itself
    if (hardware) {
        check_all(); // Check others
    }
}

/**
 * @brief Check the entire list and if all instances are kicked, reload hardware.
 */
void MultiWatchdog::check_all() {
    if (initialized == false) {
        return;
    }

    ///@note No __disable_irq is used here.
    /// Watchdog doesn't mind that it may accidentally get reloaded twice.

    // Check the entire list
    for (MultiWatchdog *item = list; item != nullptr; item = item->next) {
        if (item->mark == 0) // Someone didn't kick
        {
            return;
        }
    }

    // Clear the entire list
    for (MultiWatchdog *item = list; item != nullptr; item = item->next) {
        item->mark = 0;
    }

    // Reload hardware
    HAL_IWDG_Refresh(&hiwdg);
}
} // namespace hal
