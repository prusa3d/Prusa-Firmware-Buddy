#include <stdint.h>
#include "sound_enum.h"

/*!
 * Simple Sound class
 * This class just play sound types/signals and read & store sound mode which user can choose from Settings.
 * Every mode then have different settings for they sound signals.
 */
class Sound {
public:
    eSOUND_MODE eSoundMode;

    /// we want this as a singleton
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

    uint32_t _duration; ///< live variable used for meassure
    uint32_t duration;  ///< added variable to set _duration for repeating
    uint8_t repeat;     ///< how many times is sound played
    float frequency;    ///< frequency of sound signal (0-1000)
    float volume;       ///< volume of sound signal (0-1)

    static constexpr float volumeInit = 0.5F;
    /// values of sound signals - frequencies, volumes, durations
    uint32_t durations[eSOUND_TYPE_count] = { 100, 500, 200, 500, 50, 100, 100 };
    float frequencies[eSOUND_TYPE_count] = { 900.F, 600.F, 950.F, 999.F, 800.F, 500.F, 999.F };
    float volumes[eSOUND_TYPE_count] = { volumeInit, volumeInit, volumeInit, volumeInit, 0.25F, 0.25F, volumeInit };

    /// array of usable types (eSOUND_TYPE) of every sound modes (eSOUND_MODE)
    const eSOUND_TYPE onceTypes[4] = { eSOUND_TYPE_Start, eSOUND_TYPE_ButtonEcho, eSOUND_TYPE_StandardPrompt, eSOUND_TYPE_CriticalAlert };
    const eSOUND_TYPE loudTypes[5] = { eSOUND_TYPE_Start, eSOUND_TYPE_ButtonEcho, eSOUND_TYPE_StandardPrompt, eSOUND_TYPE_StandardAlert, eSOUND_TYPE_CriticalAlert };
    const eSOUND_TYPE silentTypes[3] = { eSOUND_TYPE_Start, eSOUND_TYPE_StandardAlert, eSOUND_TYPE_CriticalAlert };
    const eSOUND_TYPE assistTypes[7] = { eSOUND_TYPE_Start, eSOUND_TYPE_ButtonEcho, eSOUND_TYPE_StandardPrompt, eSOUND_TYPE_StandardAlert, eSOUND_TYPE_EncoderMove, eSOUND_TYPE_BlindAlert, eSOUND_TYPE_CriticalAlert };

    /// signals repeats - how many times will sound signals repeat (-1 is infinite)
    const int onceRepeats[4] = { 1, 1, 1, -1 };
    const int loudRepeats[5] = { 1, 1, -1, 3, -1 };
    const int silentRepeats[3] = { 1, 1, -1 };
    const int assistRepeats[7] = { 1, 1, -1, 3, 1, 1, -1 };

private:
    Sound();
    ~Sound() {};

    /// main fnc
    void init();
    void saveMode();
    void _sound(int rep, float frq, uint32_t dur, float vol);
    void _playSound(eSOUND_TYPE sound, const eSOUND_TYPE types[], const int repeats[], int size);
};
