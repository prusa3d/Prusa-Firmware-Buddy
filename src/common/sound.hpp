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
    void _playSound(eSOUND_TYPE type, eSOUND_MODE mode);
    void nextRepeat();
    float real_volume(int displayed_volume); ///< converts displayed / saved volume to volume used by beeper
    uint8_t displayed_volume(float real_volume); ///< converts beeper volume to displayed / saved one

    int16_t duration_active = 0; ///< live variable used for measure
    int16_t duration_set = 0; ///< added variable to set duration_ for repeating
    int repeat = 0; ///< how many times is sound played
    float frequency = 100.0f; ///< frequency of sound signal (0-1000)
    float volume = volumeInit; ///< volume of sound signal (0-1)
    float varVolume = 0; ///< varVolume is float 0-1 if it's not on One Louder (then it's 11)
    int16_t delay_active = 0; ///< live variable used for delay measure
    int16_t delay_set = 100; ///< added variable for delay between beeps
    eSOUND_MODE eSoundMode = eSOUND_MODE::_default_sound; ///< current mode

public:
    /// main constant of main volume which is maximal volume that we allow
    static constexpr float volumeInit = 0.35F;
};
