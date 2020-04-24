#include "sound_enum.h"

#ifdef __cplusplus
extern "C" {
#endif

// Global variable inicialization flag for HAL tick in appmain.cpp.
// HAL tick will start before eeprom is inicialized and Sound class is depending on that.
// See - appmain.cpp -> app_tim14_tick
extern int SOUND_INIT;

// C wrapping for old GUI parts needed.
extern eSOUND_MODE Sound_GetMode();
extern void Sound_SetMode(eSOUND_MODE eSMode);
extern void Sound_Play(eSOUND_TYPE eSoundType);
extern void Sound_Stop();
extern void Sound_Update1ms();

#ifdef __cplusplus
}
#endif
