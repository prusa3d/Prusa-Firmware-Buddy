// marlin_host.h
#ifndef _MARLIN_HOST_H
#define _MARLIN_HOST_H

#include <inttypes.h>

#define HOST_PROMPT_MAX_BUTTONS 4

// host prompts
#define MARLIN_HOSTPROMPT_LoadFilament 0x01
#define MARLIN_HOSTPROMPT_Paused       0x02

// host prompt buttons
#define MARLIN_HOSTPROMPT_BTN_Continue  0x01
#define MARLIN_HOSTPROMPT_BTN_PurgeMore 0x02

typedef enum {
    HOST_PROMPT_None = 0,
    HOST_PROMPT_Paused,
    HOST_PROMPT_FilamentRunout,
    HOST_PROMPT_LoadFilament,
    HOST_PROMPT_END,
} host_prompt_type_t;

typedef enum {
    HOST_PROMPT_BTN_None = 0,
    HOST_PROMPT_BTN_Continue,
    HOST_PROMPT_BTN_PurgeMore,
    HOST_PROMPT_BTN_DisableRunout,
    HOST_PROMPT_BTN_END,
} host_prompt_button_t;

// not used
typedef enum {
    HOST_PROMPT_RSN_NOT_DEFINED,
    HOST_PROMPT_RSN_FILAMENT_RUNOUT,
    HOST_PROMPT_RSN_USER_CONTINUE,
    HOST_PROMPT_RSN_FILAMENT_RUNOUT_REHEAT,
    HOST_PROMPT_RSN_PAUSE_RESUME,
    HOST_PROMPT_RSN_INFO,
} host_prompt_reason_t;

#pragma pack(push)
#pragma pack(1)

typedef struct _marlin_host_prompt_t {
    host_prompt_type_t type;
    uint8_t button_count;
    host_prompt_button_t button[HOST_PROMPT_MAX_BUTTONS];
} marlin_host_prompt_t;

#pragma pack(pop)

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern const char *marlin_host_prompt_get_text(host_prompt_type_t type);

extern host_prompt_type_t marlin_host_prompt_by_text(const char *text);

extern const char *marlin_host_prompt_button_get_text(host_prompt_button_t button);

extern host_prompt_button_t marlin_host_prompt_button_by_text(const char *text);

extern uint32_t marlin_host_prompt_encode(marlin_host_prompt_t *prompt);

extern void marlin_host_prompt_decode(uint32_t ui32, marlin_host_prompt_t *prompt);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // _MARLIN_HOST_H
