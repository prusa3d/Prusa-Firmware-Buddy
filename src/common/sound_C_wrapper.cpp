#include "sound_C_wrapper.h"
#include "sound.h"

extern "C" {
    void Sound_SetMode(eSOUND_MODE eSMode) { Sound::getInstance()->setMode(eSMode); }
    void Sound_DoSound(eSOUND_TYPE eSoundType) { Sound::getInstance()->doSound(eSoundType); }
    eSOUND_MODE Sound_GetMode(){
        return Sound::getInstance()->getMode();
        // return Sound.eS
        // Sound* esm = Sound::getInstance();
        // return esm->eSoundMode;
        // return Sound::getInstance()->eSoundMode;
    }
} //extern "C"
