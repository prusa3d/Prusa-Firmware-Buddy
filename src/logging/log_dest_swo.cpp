#include "log_dest_swo.h"

#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "stm32f4xx_hal.h"
#include "log_dest_shared.h"

static bool swo_is_enabled() {
    return (((ITM->TCR & ITM_TCR_ITMENA_Msk) != 0UL) && // ITM enabled
        ((ITM->TER & 1UL) != 0UL)); // ITM Port #0 enabled
}

/// Send one character over SWO (blocking)
static void swo_put_char(char character, [[maybe_unused]] void *arg) {
    while (ITM->PORT[0U].u32 == 0UL) {
        __NOP();
    }
    ITM->PORT[0U].u8 = (uint8_t)character;
}

void swo_log_event(log_event_t *event) {
    if (!swo_is_enabled()) {
        return;
    }
    log_format_simple(event, swo_put_char, NULL);
    swo_put_char('\r', NULL);
    swo_put_char('\n', NULL);
}
