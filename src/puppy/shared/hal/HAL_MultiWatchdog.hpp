#pragma once

#include <stdint.h>
#include <device/hal.h>
#include <device/cmsis.h>
#include <bsod.h>

namespace hal {

/**
 * @brief Class to gather watchdog kicks from multiple threads.
 * Each instance embeds itself to a static linked list.
 * Only if all instances kicked can the hardware watchdog be reloaded.
 */
class MultiWatchdog {
    static MultiWatchdog *list; ///< Beginning of the list
    static bool initialized; ///< Marks when the hardware was initted and started
    static IWDG_HandleTypeDef hiwdg; ///< HAL IWDG handle, including config

    MultiWatchdog *next
        = nullptr; ///< Continuation of a list of all watchdog instances
    uint8_t mark = false; ///< This mark is nonzero if this instance was kicked

public:
    /**
     * @brief Create an instance of a watchdog and add it to the global list.
     */
    MultiWatchdog();

    /**
     * @brief Watchdog cannot be destroyed.
     * The destructor is not deleted as watchdog instance can be created on stack,
     * but that function can never end.
     */
    ~MultiWatchdog() {
        bsod("Watchdog deleted");
    }

    /**
     * @brief Initialize hardware watchdog.
     * This needs to be done only once for all instances.
     */
    static void init();

    /**
     * @brief Refresh one instance of the watchdog.
     * @param hardware true to reset hardware, false to only mark (another instance must reset the hardware)
     */
    void kick(bool hardware = true);

private:
    /**
     * @brief Check the entire list and if all instances are kicked, reload hardware.
     */
    static void check_all();
};
} // namespace hal
