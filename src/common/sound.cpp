#include "sound.hpp"
#include "hwio.h"
#include <configuration_store.hpp>

eSOUND_MODE Sound_GetMode() { return Sound::getInstance().getMode(); }
int Sound_GetVolume() { return Sound::getInstance().getVolume(); }
void Sound_SetMode(eSOUND_MODE eSMode) { Sound::getInstance().setMode(eSMode); }
void Sound_SetVolume(int volume) { Sound::getInstance().setVolume(volume); }
void Sound_Play(eSOUND_TYPE eSoundType) { Sound::getInstance().play(eSoundType); }
void Sound_Stop() { Sound::getInstance().stop(); }
void Sound_Update1ms() {
    Sound::getInstance().update1ms();
}

/*!
 * Sound signals implementation
 * Simple sound implementation supporting few sound modes and having different sound types.
 * [Sound] is updated every 1ms with tim14 tick from [appmain.cpp] for measured durations of sound signals for non-blocking GUI.
 * Beeper is controlled over [hwio_buddy_2209_02.c] functions for beeper.
 */

void Sound::restore_from_eeprom() {
    // Restore mode
    eSOUND_MODE eeprom_mode = config_store().sound_mode.get();
    if (eeprom_mode != eSOUND_MODE::UNDEF) {
        setMode(eeprom_mode);
    }
    // Restore volume
    varVolume = real_volume(config_store().sound_volume.get());
}

eSOUND_MODE Sound::getMode() const {
    return eSoundMode;
}

void Sound::setMode(eSOUND_MODE eSMode) {
    eSoundMode = eSMode;
    saveMode();
}

void Sound::setVolume(int vol) {
    varVolume = real_volume(vol);
    saveVolume();
}

/// Store new Sound mode value into a EEPROM. Stored value size is 1byte
void Sound::saveMode() {
    config_store().sound_mode.set(eSoundMode);
}

/// Store new Sound VOLUME value into a EEPROM.
void Sound::saveVolume() {
    config_store().sound_volume.set(displayed_volume(varVolume));
}

/// [stopSound] is in this moment just for stopping infinitely repeating sound signal in LOUD & ASSIST mode
void Sound::stop() {
    frequency = 100.F;
    duration_active = 0;
    duration_set = 0;
    repeat = 0;
    delay_active = 0;
}

void Sound::_playSound(eSOUND_TYPE sound, const eSOUND_TYPE types[],
    const int8_t repeats[], const int16_t durations[], const int16_t delays[], unsigned size) {
    for (unsigned i = 0; i < size; i++) {
        eSOUND_TYPE type = types[i];
        if (type == sound) {
            _sound(repeats[i], frequencies[(size_t)type],
                durations[i], delays[i], volumes[(size_t)type], forced[(size_t)type]);
            break;
        }
    }
}

/*!
 * Generag [play] method with sound type parameter where dependetly on set mode is played.
 * Every mode handle just his own signal types.
 */
void Sound::play(eSOUND_TYPE eSoundType) {
    int t_size = 0;
    eSOUND_MODE mode = eSoundMode;

    if (eSoundType == eSOUND_TYPE::CriticalAlert)
        mode = eSOUND_MODE::LOUD;

    switch (mode) {
    case eSOUND_MODE::ONCE:
        t_size = sizeof(onceTypes) / sizeof(onceTypes[0]);
        _playSound(eSoundType, onceTypes, onceRepeats, onceDurations, onceDelays, t_size);
        break;
    case eSOUND_MODE::SILENT:
        t_size = sizeof(silentTypes) / sizeof(silentTypes[0]);
        _playSound(eSoundType, silentTypes, silentRepeats, silentDurations, silentDelays, t_size);
        break;
    case eSOUND_MODE::ASSIST:
        t_size = sizeof(assistTypes) / sizeof(assistTypes[0]);
        _playSound(eSoundType, assistTypes, assistRepeats, assistDurations, assistDelays, t_size);
        break;
    case eSOUND_MODE::LOUD:
    default:
        t_size = sizeof(loudTypes) / sizeof(loudTypes[0]);
        _playSound(eSoundType, loudTypes, loudRepeats, loudDurations, loudDelays, t_size);
        break;
    }
}

/// Generic [_sound] method with setting values and repeating logic
void Sound::_sound(int rep, float frq, int16_t dur, int16_t del, [[maybe_unused]] float vol, [[maybe_unused]] bool f) {
    /// forced non-repeat sounds - can be played when another
    /// repeating sound is playing
    float tmpVol;

#if BOARD_IS_BUDDY
    if (varVolume > 1) {
        tmpVol = 1.F;
    } else {
        tmpVol = f ? 0.3F : (vol * varVolume) * 0.3F;
    }
#else
    tmpVol = varVolume;
#endif
    if (rep == 1) {
        singleSound(frq, dur, tmpVol);
    } else {
        /// if sound is already playing, then don't interrupt
        if (repeat == -1 || repeat > 1) {
            return;
        }

        /// store ACTIVE variables for timing method
        repeat = rep;
        frequency = frq;
        duration_set = dur;
        delay_set = del;
        volume = tmpVol;
        /// for BSOD debugging
        if (eSoundMode == eSOUND_MODE::DEBUG) {
            volume = 0;
        }

        /// end previous beep
        hwio_beeper_notone();
        nextRepeat();
    }
}

/// Another repeat of sound signal. Just set live variable with duration_set of the beep and play it
void Sound::nextRepeat() {
    duration_active = duration_set;
    delay_active = 1;
    if (repeat > 0 || repeat == -1) {
        repeat = repeat > 0 ? repeat - 1 : repeat;
        delay_active = delay_set;
        hwio_beeper_tone2(frequency, duration_set, volume);
    }
}

float Sound::real_volume(int displayed_volume) {
#if BOARD_IS_BUDDY
    return displayed_volume == 11 ? displayed_volume : displayed_volume / 10.F;
#else
    return displayed_volume == 0 ? 0 : 1.51F - displayed_volume / 2.F;
#endif
}

uint8_t Sound::displayed_volume(float real_volume) {
#if BOARD_IS_BUDDY
    return real_volume > 1.1F ? real_volume : real_volume * 10.F;
#else
    return real_volume == 0 ? 0 : -(real_volume - 1.51F) * 2.F;
#endif
}

/// starts single sound when it's not playing another
/// this is usable when some infinitely repeating sound is playing.
void Sound::singleSound(float frq, int16_t dur, float vol) {
    if (duration_active <= 0) {
        hwio_beeper_notone();
        hwio_beeper_tone2(frq, dur, vol);
    }
}

/*!
 * Update method to control duration of sound signals and repeating count.
 * When variable [repeat] is -1, then repeating will be infinite until [stopSound] is called.
 */
void Sound::update1ms() {
    /// -- timing logic without osDelay for repeating Beep(s)
    duration_active = duration_active <= 0 ? 0 : duration_active - 1;
    if (duration_active <= 0) {
        if (--delay_active <= 0) {
            if (repeat > 0 || repeat == -1) {
                nextRepeat();
            }
        }
    }

    /// calling hwio update fnc
    hwio_update_1ms();
}
