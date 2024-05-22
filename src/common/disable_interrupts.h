/**
 * @file
 *
 */

#pragma once

#include <stdint.h>
#include <cmsis_gcc.h>

namespace buddy {

/**
 * @brief Disable all maskable interrupts
 *
 * Disabling all interrupts may be needed in case of dealing with stepper data
 * or in case timing with nanosecond precision is needed.
 *
 * In other cases use CriticalSection instead.
 *
 * Interrupts are disabled when object is constructed by its default constructor
 * and resumed to original state once destroyed.
 *
 * When the motors are moving disabling all interrupts for more than
 *
 * - 500 ns frequently may produce motor noise
 * (Induce stepping error bigger than 1% at 200 mm/s 200 step motor 16 microsteps or 400 step motor 8 microsteps)
 *
 * - 50 us produce multiple microsteps done at once (noise, may cause motor over-current in no-current-feedback-mode)
 * - 400 us may cause motor stall (400 step motors)
 * - 800 us -//- (200 step motors)
 *
 * Example usage:
 * @code
 * void singleCriticalSectionFunction() {
 *     // non-critical code
 *     {
 *         buddy::DisableInterrupts disable_interrupts;
 *         // critical code
 *     }
 *     // non-critical code
 * }
 * @endcode
 *
 * Example multiple critical sections in single function:
 * @code
 * void multipleCriticalSectionsFunction() {
 *     buddy::DisableInterrupts interrupts(false);
 *     // non-critical code
 *
 *     interrupts.disable();
 *     // critical code
 *     interrupts.resume();
 *
 *     // non-critical code
 *
 *     interrupts.disable();
 *     // critical code
 *     interrupts.resume();
 *
 *     // non-critical code
 *
 *     interrupts.disable();
 *     // critical code
 * }
 * @endcode
 */
class DisableInterrupts {
public:
    DisableInterrupts(bool disableNow = true)
        : m_primask(__get_PRIMASK()) {
        if (disableNow) {
            disable();
        }
    }

    ~DisableInterrupts() {
        resume();
    }

    void disable() {
        __disable_irq();
    }

    void resume() {
        __set_PRIMASK(m_primask);
    }

    DisableInterrupts(const DisableInterrupts &other) = delete;
    DisableInterrupts &operator=(const DisableInterrupts &other) = delete;

private:
    uint32_t m_primask;
};

} // namespace buddy
