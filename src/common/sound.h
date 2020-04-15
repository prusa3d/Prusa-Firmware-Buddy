#include <stdint.h>
#include "sound_enum.h"

class Sound {
    public:
	Sound();
        eSOUND_MODE eSoundMode;
       
	inline static Sound &getInstance(){
	    static Sound s;
	    return s;
	}
        eSOUND_MODE getMode();

        void doSound(eSOUND_TYPE eSoundType);
        void setMode(eSOUND_MODE eSMode);
        void stopSound();
        void soundUpdate1ms();
        void nextRepeat();

        uint32_t _duration;
        uint32_t duration;
        uint8_t repeat;
        double frequency;
        double volume;
    private:
        // -- main fnc
        void soundInit();
        void saveMode();
        void _sound(int rep, float frq, uint32_t del, float vol);

        // -- sound types
        void soundStart(int rep, uint32_t del);
        void soundButtonEcho(int rep, uint32_t del);
        void soundStandardPrompt(int rep, uint32_t del);
        void soundStandardAlert(int rep, uint32_t del);
        void soundEncoderMove(int rep, uint32_t del);
        void soundBlindAlert(int rep, uint32_t del);
};
