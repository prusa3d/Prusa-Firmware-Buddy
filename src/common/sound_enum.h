#pragma once
#include <stdint.h>

enum class eSOUND_MODE : uint8_t {
    ONCE,
    LOUD,
    SILENT,
    ASSIST,
#ifdef _DEBUG
    DEBUG,
#endif
    _count,
    _last = _count - 1,
    _undef = 0xFF,
    _default_sound = LOUD
};

enum class eSOUND_TYPE : uint8_t {
    ButtonEcho,
    StandardPrompt,
    StandardAlert,
    CriticalAlert,
    EncoderMove,
    BlindAlert,
    Start,
    SingleBeep,
    WaitingBeep,
    SingleBeepAlwaysLoud, // Single beep that ignores eSOUND_MODE settings, so we can play the sound in self tests
};
