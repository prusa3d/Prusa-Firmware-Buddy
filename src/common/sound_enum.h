#pragma once

#pragma pack(push)
#pragma pack(1)

typedef enum {
    eSOUND_MODE_ONCE,
    eSOUND_MODE_LOUD,
    eSOUND_MODE_SILENT,
    eSOUND_MODE_ASSIST,
    eSOUND_MODE_NULL = 0xFF,
    eSOUND_MODE_DEFAULT = eSOUND_MODE_LOUD
} eSOUND_MODE;

typedef enum {
    eSOUND_TYPE_ButtonEcho,
    eSOUND_TYPE_StandardPrompt,
    eSOUND_TYPE_StandardAlert,
    eSOUND_TYPE_CriticalAlert,
    eSOUND_TYPE_EncoderMove,
    eSOUND_TYPE_BlindAlert,
    eSOUND_TYPE_Start
} eSOUND_TYPE;

#pragma pack(pop)
