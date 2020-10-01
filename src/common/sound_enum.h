#pragma once
#include <stdint.h>

enum class eSOUND_MODE : uint8_t {
    ONCE,
    LOUD,
    SILENT,
    ASSIST,
    UNDEF = 0xFF,
    DEFAULT = LOUD
};

enum eSOUND_TYPE : uint8_t {
    ButtonEcho,
    StandardPrompt,
    StandardAlert,
    CriticalAlert,
    EncoderMove,
    BlindAlert,
    Start,
    SingleBeep,
    count
};
