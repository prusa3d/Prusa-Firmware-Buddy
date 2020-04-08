#include <stdint.h>
#include "sound_enum.h"

class Sound
{
    public:
        eSOUND_MODE eSoundMode;
        static Sound* getInstance();
        void doSound(eSOUND_TYPE eSoundType);
        void setMode(eSOUND_MODE eSMode);
        eSOUND_MODE getMode();
    private:
        // -- singleton
        Sound(){};
        // Sound(Sound const&){};
        // Sound& operator=(Sound const&){};
        static Sound* m_pInstance;

        // -- main fnc
        void soundInit();
        void saveMode();
        void _sound(int rep, float frq, uint32_t del, float vol);

        // -- sound types
        void soundButtonEcho(int rep, uint32_t del);
        void soundStandardPrompt();
        void soundStandardAlert();
        void soundEncoderMove();
        void soundBlindAlert();
};

// extern "C" void Sound_SetMode(eSOUND_MODE eSMode){
//     Sound::getInstance()->setMode(eSMode);
// }
// extern "C" void Sound_DoSound(eSOUND_TYPE eSoundType){
//     Sound::getInstance()->doSound(eSoundType);
// }

// #endif // -- SOUND_H
// #endif //__cplusplus

// #pragma pack(pop)