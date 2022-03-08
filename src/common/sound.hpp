#pragma once

#include <stdint.h>
#include "sound_enum.h"

// Global variable initialization flag for HAL tick in appmain.cpp.
// HAL tick will start before eeprom is initialized and Sound class is depending on that.
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
    int getVolume() { return displayed_volume(varVolume); }

    void setMode(eSOUND_MODE eSMode);
    void setVolume(int vol);

    void play(eSOUND_TYPE eSoundType);
    void stop();
    void update1ms();
    void singleSound(float frq, int16_t dur, float vol);

private:
    Sound();
    ~Sound() {};

    /// main fnc
    void init();
    void saveMode();
    void saveVolume(); // + one louder
    void _sound(int rep, float frq, int16_t dur, int16_t del, float vol, bool f);
    void _playSound(eSOUND_TYPE sound, const eSOUND_TYPE types[], const int8_t repeats[], const int16_t durations[], const int16_t delays[], unsigned size);
    void nextRepeat();
    float real_volume(int displayed_volume);     ///< converts displayed / saved volume to volume used by beeper
    uint8_t displayed_volume(float real_volume); ///< converts beeper volume to displayed / saved one

    int16_t duration_active; ///< live variable used for meassure
    int16_t duration_set;    ///< added variable to set duration_ for repeating
    int repeat;              ///< how many times is sound played
    float frequency;         ///< frequency of sound signal (0-1000)
    float volume;            ///< volume of sound signal (0-1)
    float varVolume;         ///< varVolume is float 0-1 if it's not on One Louder (then it's 11)
    int16_t delay_active;    ///< live variable used for delay measure
    int16_t delay_set;       ///< added variable for delay betwen beeps

    /// main constant of main volume which is maximal volume that we allow
    static const float volumeInit;

    /// values of sound signals - frequencies, volumes, durations
    static const int16_t durations[eSOUND_TYPE::count];
    static const float frequencies[eSOUND_TYPE::count];
    static const float volumes[eSOUND_TYPE::count];

    /// forced sound types - ignores volume settings
    static const bool forced[eSOUND_TYPE::count];

    /// array of usable types (eSOUND_TYPE) of every sound modes (eSOUND_MODE)
    static const eSOUND_TYPE onceTypes[];
    static const eSOUND_TYPE loudTypes[];
    static const eSOUND_TYPE silentTypes[];
    static const eSOUND_TYPE assistTypes[];

    /// signals repeats - how many times will sound signals repeat (-1 is infinite)
    static const int8_t onceRepeats[];
    static const int8_t loudRepeats[];
    static const int8_t silentRepeats[];
    static const int8_t assistRepeats[];

    /// delays for repeat sounds
    static const int16_t onceDelays[];
    static const int16_t loudDelays[];
    static const int16_t silentDelays[];
    static const int16_t assistDelays[];

    /// durations for sounds modes
    static const int16_t onceDurations[];
    static const int16_t loudDurations[];
    static const int16_t silentDurations[];
    static const int16_t assistDurations[];

    eSOUND_MODE eSoundMode;
};
