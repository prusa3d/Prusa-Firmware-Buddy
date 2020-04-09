#include "sound_C_wrapper.h"
#include "sound.h"

extern "C" {
    eSOUND_MODE Sound_GetMode(){ return Sound::getInstance()->getMode(); }
    void Sound_SetMode(eSOUND_MODE eSMode) { Sound::getInstance()->setMode(eSMode); }
    void Sound_DoSound(eSOUND_TYPE eSoundType) { Sound::getInstance()->doSound(eSoundType); }
    void Sound_StopSound() { Sound::getInstance()->stopSound(); }
    void Sound_UpdateSound1ms(){ Sound::soundUpdate1ms(); }
} //extern "C"
