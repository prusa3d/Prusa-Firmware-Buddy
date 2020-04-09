#include <stdint.h>
#include "sound_enum.h"

class Sound
{
    public:
        static Sound& s;
        eSOUND_MODE eSoundMode;
        
        static Sound* getInstance();
        eSOUND_MODE getMode();

        void doSound(eSOUND_TYPE eSoundType);
        void setMode(eSOUND_MODE eSMode);
        void stopSound();
        static void soundUpdate1ms();
        static void nextRepeat();

        static uint32_t _duration;
        static uint32_t duration;
        static uint8_t repeat;
        static double frequency;
        static double volume;
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
