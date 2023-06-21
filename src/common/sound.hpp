#pragma once

#include <stdint.h>
#include "sound_enum.h"

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

    /**
     * Restore sound settings configuration from eeprom
     *
     * Until this is called the sound uses default settings. Needs eeprom to load the configuration.
     */
    void restore_from_eeprom();

    void play(eSOUND_TYPE eSoundType);
    void stop();
    void update1ms();
    void singleSound(float frq, int16_t dur, float vol);

private:
    Sound() = default;
    ~Sound() = default;

    /// main fnc
    void saveMode();
    void saveVolume(); // + one louder
    void _sound(int rep, float frq, int16_t dur, int16_t del, float vol, bool f);
    void _playSound(eSOUND_TYPE sound, const eSOUND_TYPE types[], const int8_t repeats[], const int16_t durations[], const int16_t delays[], unsigned size);
    void nextRepeat();
    float real_volume(int displayed_volume);             ///< converts displayed / saved volume to volume used by beeper
    uint8_t displayed_volume(float real_volume);         ///< converts beeper volume to displayed / saved one

    int16_t duration_active = 0;                         ///< live variable used for measure
    int16_t duration_set = 0;                            ///< added variable to set duration_ for repeating
    int repeat = 0;                                      ///< how many times is sound played
    float frequency = 100.0f;                            ///< frequency of sound signal (0-1000)
    float volume = volumeInit;                           ///< volume of sound signal (0-1)
    float varVolume = 0;                                 ///< varVolume is float 0-1 if it's not on One Louder (then it's 11)
    int16_t delay_active = 0;                            ///< live variable used for delay measure
    int16_t delay_set = 100;                             ///< added variable for delay between beeps
    eSOUND_MODE eSoundMode = eSOUND_MODE::DEFAULT_SOUND; ///< current mode

    /// main constant of main volume which is maximal volume that we allow
    static constexpr float volumeInit = 0.35F;

    /// frequency of signals in ms
    static constexpr float frequencies[eSOUND_TYPE::count] = { 900.F, 600.F, 950.F, 999.F, 800.F, 500.F, 999.F, 950.F, 800.F };

    /// volumes of signals in ms
    static constexpr float volumes[eSOUND_TYPE::count] = {
        Sound::volumeInit,
        Sound::volumeInit,
        Sound::volumeInit,
        Sound::volumeInit,
        0.175F,
        0.175F,
        Sound::volumeInit,
        Sound::volumeInit,
        Sound::volumeInit
    };

    /// forced types of sounds - mainly for ERROR sounds. Ignores volume settings.
    static constexpr bool forced[eSOUND_TYPE::count] = { false, false, false, true, false, false, false, false, false };

    /// array of usable types (eSOUND_TYPE) of every sound modes (eSOUND_MODE)
    static constexpr eSOUND_TYPE onceTypes[] = {
        eSOUND_TYPE::Start,
        eSOUND_TYPE::ButtonEcho,
        eSOUND_TYPE::StandardPrompt,
        eSOUND_TYPE::CriticalAlert,
        eSOUND_TYPE::SingleBeep,
        eSOUND_TYPE::WaitingBeep
    };
    static constexpr eSOUND_TYPE loudTypes[] = {
        eSOUND_TYPE::Start,
        eSOUND_TYPE::ButtonEcho,
        eSOUND_TYPE::StandardPrompt,
        eSOUND_TYPE::StandardAlert,
        eSOUND_TYPE::CriticalAlert,
        eSOUND_TYPE::SingleBeep,
        eSOUND_TYPE::WaitingBeep
    };
    static constexpr eSOUND_TYPE silentTypes[] = {
        eSOUND_TYPE::Start,
        eSOUND_TYPE::StandardAlert,
        eSOUND_TYPE::CriticalAlert
    };
    static constexpr eSOUND_TYPE assistTypes[] = {
        eSOUND_TYPE::Start,
        eSOUND_TYPE::ButtonEcho,
        eSOUND_TYPE::StandardPrompt,
        eSOUND_TYPE::StandardAlert,
        eSOUND_TYPE::EncoderMove,
        eSOUND_TYPE::BlindAlert,
        eSOUND_TYPE::CriticalAlert,
        eSOUND_TYPE::SingleBeep,
        eSOUND_TYPE::WaitingBeep
    };

    /// signals repeats - how many times will sound signals repeat (-1 is infinite)
    static constexpr int8_t onceRepeats[] = { 1, 1, 1, -1, 1, 1 };
    static constexpr int8_t loudRepeats[] = { 1, 1, -1, 3, -1, 1, -1 };
    static constexpr int8_t silentRepeats[] = { 1, 1, -1 };
    static constexpr int8_t assistRepeats[] = { 1, 1, -1, 3, 1, 1, -1, 1, -1 };

    /// delays for repeat sounds (ms)
    static constexpr int16_t onceDelays[] = { 1, 1, 1, 250, 1 };
    static constexpr int16_t loudDelays[] = { 1, 1, 1, 1, 250, 1, 2000 };
    static constexpr int16_t silentDelays[] = { 1, 1, 250 };
    static constexpr int16_t assistDelays[] = { 1, 1, 1, 1, 1, 1, 250, 1, 2000 };

    /// durations for sound modes
    static constexpr int16_t onceDurations[] = {
        100, 100, 500, 500, 800, 800
    };
    static constexpr int16_t loudDurations[] = {
        100, 100, 500, 200, 500, 800, 100
    };
    static constexpr int16_t silentDurations[] = {
        100, 200, 500
    };
    static constexpr int16_t assistDurations[] = {
        100, 100, 500, 200, 10, 50, 500, 800, 100
    };
};
