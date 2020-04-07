#include "sound_C_wrapper.h"
#include "sound.h"

extern "C" {
    void Sound_SetMode(eSOUND_MODE eSMode) {
        // int i = 0;
        Sound::getInstance()->setMode(eSMode);
    }
    void Sound_DoSound(eSOUND_TYPE eSoundType) { Sound::getInstance()->doSound(eSoundType); }
} //extern "C"
