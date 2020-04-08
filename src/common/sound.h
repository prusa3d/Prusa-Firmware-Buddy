#include <stdint.h>
#include "sound_enum.h"

class Sound
{
    public:
        static Sound& s;
        eSOUND_MODE eSoundMode;
        static Sound* getInstance();
        void doSound(eSOUND_TYPE eSoundType);
        void setMode(eSOUND_MODE eSMode);
        eSOUND_MODE getMode();
    private:
        bool _inited = false;
        // -- singleton
        Sound(){};

        // -- main fnc
        void soundInit();
        void saveMode();
        void _sound(int rep, float frq, uint32_t del, float vol);

        // -- sound types
        void soundButtonEcho(int rep, uint32_t del);
        void soundStandardPrompt(int rep, uint32_t del);
        void soundStandardAlert(int rep, uint32_t del);
        void soundEncoderMove(int rep, uint32_t del);
        void soundBlindAlert(int rep, uint32_t del);
};

// extern "C" void Sound_SetMode(eSOUND_MODE eSMode){
//     Sound::getInstance()->setMode(eSMode);
// }
// extern "C" void Sound_DoSound(eSOUND_TYPE eSoundType){
//     Sound::getInstance()->doSound(eSoundType);
// }
