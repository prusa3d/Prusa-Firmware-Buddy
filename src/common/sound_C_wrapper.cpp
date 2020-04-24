#include "sound_C_wrapper.h"
#include "sound.h"

extern "C" {
int SOUND_INIT = 0;

eSOUND_MODE Sound_GetMode() { return Sound::getInstance().getMode(); }
void Sound_SetMode(eSOUND_MODE eSMode) { Sound::getInstance().setMode(eSMode); }
void Sound_Play(eSOUND_TYPE eSoundType) { Sound::getInstance().play(eSoundType); }
void Sound_Stop() { Sound::getInstance().stop(); }

// Global variable inicialization flag for HAL tick in appmain.cpp.
// HAL tick will start before eeprom is inicialized and Sound class is depending on that.
// See - appmain.cpp -> app_tim14_tick
void Sound_Update1ms() {
    if (SOUND_INIT) {
        Sound::getInstance().update1ms();
    }
}
} //extern "C"
