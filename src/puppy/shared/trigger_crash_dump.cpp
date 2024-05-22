#include "stm32g0xx.h"
#include "trigger_crash_dump.h"

void trigger_crash_dump() {
    SCB->ICSR = SCB_ICSR_NMIPENDSET_Msk;
}
