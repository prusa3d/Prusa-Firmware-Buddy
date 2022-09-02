#include "disable_interrupts.h"
using namespace buddy;

DisableInterrupts::DisableInterrupts(bool disableNow)
    : m_primask(__get_PRIMASK()) {
    if (disableNow)
        disable();
}

DisableInterrupts::~DisableInterrupts() {
    resume();
}

void DisableInterrupts::disable() {
    __disable_irq();
}

void DisableInterrupts::resume() {
    __set_PRIMASK(m_primask);
}
