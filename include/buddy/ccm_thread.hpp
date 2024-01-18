#pragma once

#include <cstdint>
#include <cmsis_os.h>
#include <stm32f4xx_hal.h>

/// @brief Create thread with stack placed in CCMRAM - Core Coupled Memory RAM.

/// Create a Thread Definition with function, priority, and stack requirements.
/// \param         name         name of the thread function.
/// \param         priority     initial priority of the thread function.
/// \param         instances    number of possible thread instances.
/// \param         stacksz      stack size (in words of StackType_t) requirements for the thread function.
///
/// @todo Thread as a parameter is a bug, not in original CMSIS.
/// @todo Instances should probably be 1 and only 1 for static (including osThreadStaticDef) allocated threads. Our code uses 0 for all.
/// @todo Size in words is a bug, should be in bytes.
///
#if (configSUPPORT_STATIC_ALLOCATION == 1)
    #define osThreadCCMDef(name, thread, priority, instances, stacksz)                        \
        static uint32_t __attribute__((section(".ccmram"))) os_thread_buffer_##name[stacksz]; \
        static StaticTask_t __attribute__((section(".ccmram"))) os_thread_control_##name;     \
        const osThreadDef_t os_thread_def_##name = { #name, (thread), (priority), (instances), (stacksz), os_thread_buffer_##name, &os_thread_control_##name }
#endif

/**
 * @brief Check if data are in CCMRAM.
 * CCMRAM cannot be accessed by DMA.
 * @param address address of data
 * @return true if data are in CCMRAM
 */
inline bool is_ccm_ram(uintptr_t address) {
    static_assert(CCMDATARAM_BASE < SRAM1_BASE, "This function uses SRAM1 as upper limit of CCMRAM");
    return address >= CCMDATARAM_BASE && address < SRAM1_BASE;
}

/**
 * @brief Check if data can be used by DMA.
 * CCMRAM cannot be accessed by DMA.
 * Use as:
 *    assert(can_be_used_by_dma(data));
 * @param address address of data
 * @return true if data can be used by DMA
 */
template <class T>
FORCE_INLINE bool can_be_used_by_dma(T *address) {
    return !is_ccm_ram(reinterpret_cast<uintptr_t>(address));
}
