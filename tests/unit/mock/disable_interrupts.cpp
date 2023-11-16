#include "disable_interrupts.h"
using namespace buddy;

DisableInterrupts::DisableInterrupts(bool disableNow)
    : m_primask(0) {
    if (disableNow) {
        disable();
    }
}

DisableInterrupts::~DisableInterrupts() {
    resume();
}

void DisableInterrupts::disable() {
}

void DisableInterrupts::resume() {
}
