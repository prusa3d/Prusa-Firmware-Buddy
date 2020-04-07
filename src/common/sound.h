#include <stdint.h>
#include "sound_enum.h"

// #pragma pack(push)
// #pragma pack(1)

// #ifndef SOUND_H

// #define SOUND_H

// typedef enum {
//     eSOUND_MODE_ONCE,
//     eSOUND_MODE_LOUD,
//     eSOUND_MODE_SILENT,
//     eSOUND_MODE_ASSIST
// } eSOUND_MODE;

// typedef enum {
//     eSOUND_TYPE_ButtonEcho,
//     eSOUND_TYPE_StandardPrompt,
//     eSOUND_TYPE_StandardAlert,
//     eSOUND_TYPE_EncoderMove,
//     eSOUND_TYPE_BlindAlert
// } eSOUND_TYPE;

// #define eSOUND_MODE_NULL 0xFF
// #define eSOUND_MODE_DEFAULT eSOUND_MODE_LOUD

extern eSOUND_MODE eSoundMode;

// extern "C" void Sound_DoSound(eSOUND_TYPE eSoundType);
// extern "C" void Sound_SetMode(eSOUND_MODE eSoundMode);

class Sound
{
    public:
        static Sound* getInstance();
        void doSound(eSOUND_TYPE eSoundType);
        void setMode(eSOUND_MODE eSMode);
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