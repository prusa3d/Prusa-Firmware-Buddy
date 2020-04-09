#include "sound_enum.h"

#ifdef __cplusplus
extern "C" {
#endif

extern eSOUND_MODE Sound_GetMode();
extern void Sound_SetMode(eSOUND_MODE eSMode);
extern void Sound_DoSound(eSOUND_TYPE eSoundType);
extern void Sound_StopSound();
extern void Sound_UpdateSound1ms();

#ifdef __cplusplus
}
#endif