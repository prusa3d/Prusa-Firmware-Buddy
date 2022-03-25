#include "sound.hpp"
#include "hwio.h"
#include "eeprom.h"

static bool SOUND_INIT = false;

/// main constant of main volume which is maximal volume that we allow
const float Sound::volumeInit = 0.35F;

/// durations of signals in ms
// const int16_t Sound::durations[eSOUND_TYPE::count] = { 100, 500, 200, 500,
// 10, 50, 100, 800, 100 };
/// frequency of signals in ms
const float Sound::frequencies[eSOUND_TYPE::count] = { 900.F, 600.F, 950.F,
    999.F, 800.F, 500.F, 999.F, 950.F, 800.F };
/// volumes of signals in ms
const float Sound::volumes[eSOUND_TYPE::count] = {
    Sound::volumeInit, Sound::volumeInit, Sound::volumeInit, Sound::volumeInit,
    0.175F, 0.175F, Sound::volumeInit, Sound::volumeInit, Sound::volumeInit
};
/// forced types of sounds - mainly for ERROR sounds. Ignores volume settings.
const bool Sound::forced[eSOUND_TYPE::count] = { false, false, false, true, false, false, false, false, false };

/// array of usable types (eSOUND_TYPE) of every sound modes (eSOUND_MODE)
const eSOUND_TYPE Sound::onceTypes[] = { eSOUND_TYPE::Start, eSOUND_TYPE::ButtonEcho,
    eSOUND_TYPE::StandardPrompt, eSOUND_TYPE::CriticalAlert, eSOUND_TYPE::SingleBeep,
    eSOUND_TYPE::WaitingBeep };
const eSOUND_TYPE Sound::loudTypes[] = { eSOUND_TYPE::Start, eSOUND_TYPE::ButtonEcho,
    eSOUND_TYPE::StandardPrompt, eSOUND_TYPE::StandardAlert, eSOUND_TYPE::CriticalAlert,
    eSOUND_TYPE::SingleBeep, eSOUND_TYPE::WaitingBeep };
const eSOUND_TYPE Sound::silentTypes[] = { eSOUND_TYPE::Start, eSOUND_TYPE::StandardAlert,
    eSOUND_TYPE::CriticalAlert };
const eSOUND_TYPE Sound::assistTypes[] = { eSOUND_TYPE::Start, eSOUND_TYPE::ButtonEcho,
    eSOUND_TYPE::StandardPrompt, eSOUND_TYPE::StandardAlert, eSOUND_TYPE::EncoderMove,
    eSOUND_TYPE::BlindAlert, eSOUND_TYPE::CriticalAlert, eSOUND_TYPE::SingleBeep,
    eSOUND_TYPE::WaitingBeep };

/// signals repeats - how many times will sound signals repeat (-1 is infinite)
const int8_t Sound::onceRepeats[] = { 1, 1, 1, -1, 1, 1 };
const int8_t Sound::loudRepeats[] = { 1, 1, -1, 3, -1, 1, -1 };
const int8_t Sound::silentRepeats[] = { 1, 1, -1 };
const int8_t Sound::assistRepeats[] = { 1, 1, -1, 3, 1, 1, -1, 1, -1 };

/// delays for repeat sounds (ms)
const int16_t Sound::onceDelays[] = { 1, 1, 1, 250, 1 };
const int16_t Sound::loudDelays[] = { 1, 1, 1, 1, 250, 1, 2000 };
const int16_t Sound::silentDelays[] = { 1, 1, 250 };
const int16_t Sound::assistDelays[] = { 1, 1, 1, 1, 1, 1, 250, 1, 2000 };

/// durations for sound modes
const int16_t Sound::onceDurations[] = {
    100, 100, 500, 500, 800, 800
};
const int16_t Sound::loudDurations[] = {
    100, 100, 500, 200, 500, 800, 100
};
const int16_t Sound::silentDurations[] = {
    100, 200, 500
};
const int16_t Sound::assistDurations[] = {
    100, 100, 500, 200, 10, 50, 500, 800, 100
};

eSOUND_MODE Sound_GetMode() { return Sound::getInstance().getMode(); }
int Sound_GetVolume() { return Sound::getInstance().getVolume(); }
void Sound_SetMode(eSOUND_MODE eSMode) { Sound::getInstance().setMode(eSMode); }
void Sound_SetVolume(int volume) { Sound::getInstance().setVolume(volume); }
void Sound_Play(eSOUND_TYPE eSoundType) { Sound::getInstance().play(eSoundType); }
void Sound_Stop() { Sound::getInstance().stop(); }
void Sound_Update1ms() {
    if (SOUND_INIT) {
        Sound::getInstance().update1ms();
    }
}

/*!
 * Sound signals implementation
 * Simple sound implementation supporting few sound modes and having different sound types.
 * [Sound] is updated every 1ms with tim14 tick from [appmain.cpp] for measured durations of sound signals for non-blocking GUI.
 * Beeper is controled over [hwio_buddy_2209_02.c] functions for beeper.
 */
Sound::Sound()
    : duration_active(0)
    , duration_set(0)
    , repeat(0)
    , frequency(100.F)
    , volume(volumeInit)
    , delay_active(0)
    , delay_set(100) {
    init();
}

/*!
 * Initialization of Singleton Class needs to be AFTER eeprom initialization.
 * [soundInit] is getting stored EEPROM value of his sound mode.
 * [soundInit] sets global variable [SOUND_INIT] for safe update method([soundUpdate1ms]) because tim14 tick update method is called before [eeprom.c] is initialized.
 */
void Sound::init() {
    eSoundMode = static_cast<eSOUND_MODE>(eeprom_get_ui8(EEVAR_SOUND_MODE));
    if (eSoundMode == eSOUND_MODE::UNDEF) {
        setMode(eSOUND_MODE::DEFAULT_SOUND);
    }
    varVolume = real_volume(eeprom_get_ui8(EEVAR_SOUND_VOLUME));
    /// GLOBAL FLAG set on demand when first sound method is called
    SOUND_INIT = true;
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
    eeprom_set_ui8(EEVAR_SOUND_MODE, (uint8_t)eSoundMode);
}

/// Store new Sound VOLUME value into a EEPROM.
void Sound::saveVolume() {
    eeprom_set_ui8(EEVAR_SOUND_VOLUME, displayed_volume(varVolume));
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
    switch (eSoundMode) {
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
void Sound::_sound(int rep, float frq, int16_t dur, int16_t del, float vol, bool f) {
    /// forced non-repeat sounds - can be played when another
    /// repeating sound is playing
    float tmpVol;
    if (varVolume > 1) {
        tmpVol = 1.F;
    } else {
        tmpVol = f ? 0.3F : (vol * varVolume) * 0.3F;
    }
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
        hwio_beeper_set_pwm(0, 0);
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
    return displayed_volume == 11 ? displayed_volume : displayed_volume / 10.F;
}

uint8_t Sound::displayed_volume(float real_volume) {
    return real_volume > 1.1F ? real_volume : real_volume * 10.F;
}

/// starts single sound when it's not playing another
/// this is usable when some infinitely repeating sound is playing.
void Sound::singleSound(float frq, int16_t dur, float vol) {
    if (duration_active <= 0) {
        hwio_beeper_set_pwm(0, 0);
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
