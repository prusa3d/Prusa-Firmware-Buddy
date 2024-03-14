#include "SEGGER_SYSVIEW.h"
#include "device/hal.h"

U32 SEGGER_SYSVIEW_X_GetTimestamp() {
    return HAL_GetTick();
}
