#include "sound_enum.h"

#ifdef __cplusplus
extern "C" {
#endif

// Global variable inicialization flag for HAL tick in appmain.cpp.
// HAL tick will start before eeprom is inicialized and Sound class is depending on that. See - appmain.cpp -> app_tim14_tick
extern int SOUND_INIT;

extern eSOUND_MODE Sound_GetMode();
extern void Sound_SetMode(eSOUND_MODE eSMode);
extern void Sound_DoSound(eSOUND_TYPE eSoundType);
extern void Sound_StopSound();
extern void Sound_UpdateSound1ms();

#ifdef __cplusplus
}
#endif
