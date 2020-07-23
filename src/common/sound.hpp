#include <stdint.h>
#include "sound_enum.h"

// Global variable inicialization flag for HAL tick in appmain.cpp.
// HAL tick will start before eeprom is inicialized and Sound class is depending on that.
// See - appmain.cpp -> app_tim14_tick
// uint8_t SOUND_INIT;

eSOUND_MODE Sound_GetMode();
int Sound_GetVolume();
void Sound_SetMode(eSOUND_MODE eSMode);
void Sound_SetVolume(int volume);
void Sound_Play(eSOUND_TYPE eSoundType);
void Sound_Stop();
void Sound_Update1ms();

/*!
 * Simple Sound class
 * This class just play sound types/signals and read & store sound mode which user can choose from Settings.
 * Every mode then have different settings for they sound signals.
 */
class Sound {
public:
    /// we want this as a singleton
    inline static Sound &getInstance() {
        static Sound s;
        return s;
    }
    Sound(const Sound &) = delete;
    Sound &operator=(const Sound &) = delete;

    eSOUND_MODE getMode() const;
    int getVolume();

    void setMode(eSOUND_MODE eSMode);
    void setVolume(int vol);

    void play(eSOUND_TYPE eSoundType);
    void stop();
    void update1ms();

private:
    Sound();
    ~Sound() {};

    /// main fnc
    void init();
    void saveMode();
    void saveVolume();
    void _sound(int rep, float frq, uint32_t dur, float vol);
    void _playSound(eSOUND_TYPE sound, const eSOUND_TYPE types[], const int repeats[], unsigned size);

    void nextRepeat();

    uint32_t _duration; ///< live variable used for meassure
    uint32_t duration;  ///< added variable to set _duration for repeating
    int repeat;         ///< how many times is sound played
    float frequency;    ///< frequency of sound signal (0-1000)
    float volume;       ///< volume of sound signal (0-1)
    float varVolume;    ///< variable volume set from user (0-10)
    uint32_t _delay;    ///< live variable used for delay measure
    uint32_t delay;     ///< added variable for delay betwen beeps

    static constexpr float volumeInit = 0.35F;
    /// values of sound signals - frequencies, volumes, durations
    static const uint32_t durations[eSOUND_TYPE_count];
    static const float frequencies[eSOUND_TYPE_count];
    static const float volumes[eSOUND_TYPE_count];

    /// array of usable types (eSOUND_TYPE) of every sound modes (eSOUND_MODE)
    static const eSOUND_TYPE onceTypes[5];
    static const eSOUND_TYPE loudTypes[6];
    static const eSOUND_TYPE silentTypes[3];
    static const eSOUND_TYPE assistTypes[8];

    /// signals repeats - how many times will sound signals repeat (-1 is infinite)
    static const int onceRepeats[5];
    static const int loudRepeats[6];
    static const int silentRepeats[3];
    static const int assistRepeats[8];

    eSOUND_MODE eSoundMode;
};
