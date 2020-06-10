#include <stdint.h>
#include "sound_enum.h"

// Simple Sound class
// This class just play sound types/signals and read & store sound mode which user can choose from Settings.
// Every mode then have different settings for they sound signals.
class Sound {
public:
    eSOUND_MODE eSoundMode;

    // we want this as a singleton
    inline static Sound &getInstance() {
        static Sound s;
        return s;
    }
    Sound(const Sound &) = delete;
    Sound &operator=(const Sound &) = delete;

    eSOUND_MODE getMode();

    void play(eSOUND_TYPE eSoundType);
    void setMode(eSOUND_MODE eSMode);
    void stop();
    void update1ms();
    void nextRepeat();

    uint32_t _duration;
    uint32_t duration;
    uint8_t repeat;
    double frequency;
    double volume;

    uint32_t durations[eSOUND_TYPE_count];
    double frequencies[eSOUND_TYPE_count];
    double volumes[eSOUND_TYPE_count];

		const int8_t onceRepeats[4];
		const int8_t loudRepeats[5];
		const int8_t silentRepeats[3];
		const int8_t assistRepeats[7];

    const eSOUND_TYPE onceTypes[4]= { eSOUND_TYPE_Start, eSOUND_TYPE_ButtonEcho, eSOUND_TYPE_StandardPrompt, eSOUND_TYPE_CriticalAlert };
    const eSOUND_TYPE loudTypes[5] = { eSOUND_TYPE_Start, eSOUND_TYPE_ButtonEcho, eSOUND_TYPE_StandardPrompt, eSOUND_TYPE_StandardAlert, eSOUND_TYPE_CriticalAlert };
    const eSOUND_TYPE silentTypes[3] = { eSOUND_TYPE_Start, eSOUND_TYPE_StandardAlert, eSOUND_TYPE_CriticalAlert };
    const eSOUND_TYPE assistTypes[7] = { eSOUND_TYPE_Start, eSOUND_TYPE_ButtonEcho, eSOUND_TYPE_StandardPrompt, eSOUND_TYPE_StandardAlert, eSOUND_TYPE_EncoderMove, eSOUND_TYPE_BlindAlert, eSOUND_TYPE_CriticalAlert };

private:
    Sound();
    ~Sound() {};

    // -- main fnc
    void init();
    void saveMode();
    void _sound(int rep, float frq, uint32_t del, float vol);
		void _playSound(eSOUND_TYPE sound, const eSOUND_TYPE types[], int size);

    // -- sound types
    void soundStart(int rep, uint32_t del);
    void soundButtonEcho(int rep, uint32_t del);
    void soundStandardPrompt(int rep, uint32_t del);
    void soundStandardAlert(int rep, uint32_t del);
    void soundEncoderMove(int rep, uint32_t del);
    void soundBlindAlert(int rep, uint32_t del);
    void soundCriticalAlert(int rep, uint32_t del);
};
