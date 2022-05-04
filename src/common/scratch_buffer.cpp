#include "scratch_buffer.hpp"
#include "cmsis_os.h"
#include "bsod.h"

buddy::scratch_buffer::ScratchBuffer buffer __attribute__((section(".ccmram")));
osMutexDef(buffer_mutex);
osMutexId buffer_mutex_id = 0;

buddy::scratch_buffer::Ownership::Ownership()
    : acquired(false) {
    if (buffer_mutex_id == 0) {
        buffer_mutex_id = osMutexCreate(osMutex(buffer_mutex));
    }
}

bool buddy::scratch_buffer::Ownership::acquire(bool wait) {
    if (acquired) {
        return true;
    }

    auto result = osMutexWait(buffer_mutex_id, wait ? osWaitForever : 0);
    acquired = result == osOK;
    return acquired;
}

buddy::scratch_buffer::ScratchBuffer &buddy::scratch_buffer::Ownership::get() {
    if (acquired) {
        return buffer;
    } else {
        bsod("scratchbuffer not acquired");
    }
}

void buddy::scratch_buffer::Ownership::release() {
    if (acquired) {
        osMutexRelease(buffer_mutex_id);
    }
}

buddy::scratch_buffer::Ownership::~Ownership() {
    release();
}
