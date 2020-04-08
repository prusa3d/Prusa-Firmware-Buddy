#include "sound_enum.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void Sound_SetMode(eSOUND_MODE eSMode);
extern void Sound_DoSound(eSOUND_TYPE eSoundType);
extern eSOUND_MODE Sound_GetMode();

#ifdef __cplusplus
}
#endif