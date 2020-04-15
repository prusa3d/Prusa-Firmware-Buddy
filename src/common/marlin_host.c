// marlin_host.c

#include "marlin_host.h"
#include "string.h" //strcmp

// host prompt text constants (dbg)
const char *__prompt_text[] = {
    "",                 // HOST_PROMPT_None = 0
    "Paused",           // HOST_PROMPT_Paused
    "FilamentRunout T", // HOST_PROMPT_FilamentRunout
    "LoadFilament T",   // HOST_PROMPT_LoadFilament
};

// host prompt button text constants (dbg)
const char *__prompt_button_text[] = {
    "",              // HOST_PROMPT_BTN_None = 0
    "Continue",      // HOST_PROMPT_BTN_Continue
    "PurgeMore",     // HOST_PROMPT_BTN_PurgeMore
    "DisableRunout", // HOST_PROMPT_BTN_DisableRunout
};

const char *marlin_host_prompt_get_text(host_prompt_type_t type) {
    uint8_t index = (uint8_t)type;
    if (index < HOST_PROMPT_END)
        return __prompt_text[index];
    return "";
}

host_prompt_type_t marlin_host_prompt_by_text(const char *text) {
    uint8_t index;
    for (index = 0; index < HOST_PROMPT_END; index++)
        if (strcmp(text, __prompt_text[index]) == 0)
            return (host_prompt_type_t)index;
    return HOST_PROMPT_None;
}

const char *marlin_host_prompt_button_get_text(host_prompt_button_t button) {
    uint8_t index = (uint8_t)button;
    if (index < HOST_PROMPT_BTN_END)
        return __prompt_text[index];
    return "";
}

host_prompt_button_t marlin_host_prompt_button_by_text(const char *text) {
    uint8_t index;
    for (index = 0; index < HOST_PROMPT_BTN_END; index++)
        if (strcmp(text, __prompt_button_text[index]) == 0)
            return (host_prompt_button_t)index;
    return HOST_PROMPT_BTN_None;
}

uint32_t marlin_host_prompt_encode(marlin_host_prompt_t *prompt) {
    return ((uint32_t)prompt->type & 0xf) | (((uint32_t)prompt->button_count & 0xf) << 4) | (((uint32_t)prompt->button[0] & 0xf) << 8) | (((uint32_t)prompt->button[1] & 0xf) << 12) | (((uint32_t)prompt->button[2] & 0xf) << 16) | (((uint32_t)prompt->button[3] & 0xf) << 20);
}

void marlin_host_prompt_decode(uint32_t ui32, marlin_host_prompt_t *prompt) {
    prompt->type = ui32 & 0xf;
    prompt->button_count = (ui32 >> 4) & 0xf;
    prompt->button[0] = (ui32 >> 8) & 0xf;
    prompt->button[1] = (ui32 >> 12) & 0xf;
    prompt->button[2] = (ui32 >> 16) & 0xf;
    prompt->button[3] = (ui32 >> 20) & 0xf;
}
