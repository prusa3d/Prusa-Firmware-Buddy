#include "sound.hpp"
#include "hwio.h"
#include "eeprom.h"

static bool SOUND_INIT = false;

/// durations of signals in ms
const uint32_t Sound::durations[eSOUND_TYPE::count] = { 100, 500, 200, 500, 10, 50, 100, 800 };
/// durations of signals in ms
const float Sound::frequencies[eSOUND_TYPE::count] = { 900.F, 600.F, 950.F, 999.F, 800.F, 500.F, 999.F, 950.F };
/// durations of signals in ms
const float Sound::volumes[eSOUND_TYPE::count] = { Sound::volumeInit, Sound::volumeInit, Sound::volumeInit, Sound::volumeInit, 0.175F, 0.175F, Sound::volumeInit, Sound::volumeInit };

const eSOUND_TYPE Sound::onceTypes[5] = { eSOUND_TYPE::Start, eSOUND_TYPE::ButtonEcho, eSOUND_TYPE::StandardPrompt, eSOUND_TYPE::CriticalAlert, eSOUND_TYPE::SingleBeep };
const eSOUND_TYPE Sound::loudTypes[6] = { eSOUND_TYPE::Start, eSOUND_TYPE::ButtonEcho, eSOUND_TYPE::StandardPrompt, eSOUND_TYPE::StandardAlert, eSOUND_TYPE::CriticalAlert, eSOUND_TYPE::SingleBeep };
const eSOUND_TYPE Sound::silentTypes[3] = { eSOUND_TYPE::Start, eSOUND_TYPE::StandardAlert, eSOUND_TYPE::CriticalAlert };
const eSOUND_TYPE Sound::assistTypes[8] = { eSOUND_TYPE::Start, eSOUND_TYPE::ButtonEcho, eSOUND_TYPE::StandardPrompt, eSOUND_TYPE::StandardAlert, eSOUND_TYPE::EncoderMove, eSOUND_TYPE::BlindAlert, eSOUND_TYPE::CriticalAlert, eSOUND_TYPE::SingleBeep };

const int Sound::onceRepeats[5] = { 1, 1, 1, 1, 1 };
const int Sound::loudRepeats[6] = { 1, 1, -1, 3, -1, 1 };
const int Sound::silentRepeats[3] = { 1, 1, 1 };
const int Sound::assistRepeats[8] = { 1, 1, -1, 3, 1, 1, -1, 1 };

/* const bool Sound::forced[8] = { false, false, false, false, false, true, false, false }; */

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
 * Beeper is controled over [hwio_a3ides_2209_02.c] functions for beeper.
 */
Sound::Sound()
    : _duration(0)
    , duration(0)
    , repeat(0)
    , frequency(100.F)
    , volume(volumeInit)
    , _delay(0)
    , delay(100) {
    init();
}

/*!
 * Inicialization of Singleton Class needs to be AFTER eeprom inicialization.
 * [soundInit] is getting stored EEPROM value of his sound mode.
 * [soundInit] sets global variable [SOUND_INIT] for safe update method([soundUpdate1ms]) because tim14 tick update method is called before [eeprom.c] is initialized.
 */
void Sound::init() {
    eSoundMode = static_cast<eSOUND_MODE>(variant_get_ui8(eeprom_get_var(EEVAR_SOUND_MODE)));
    if (eSoundMode == eSOUND_MODE::UNDEF) {
        setMode(eSOUND_MODE::DEFAULT);
    }
    varVolume = variant_get_ui8(eeprom_get_var(EEVAR_SOUND_VOLUME)) / 10.F;
    /// GLOBAL FLAG set on demand when first sound method is called
    SOUND_INIT = true;
}

eSOUND_MODE Sound::getMode() const {
    return eSoundMode;
}

int Sound::getVolume() {
    int retval = (varVolume * 10.F);
    return retval;
}

void Sound::setMode(eSOUND_MODE eSMode) {
    eSoundMode = eSMode;
    saveMode();
}

void Sound::setVolume(int vol) {
    varVolume = static_cast<uint8_t>(vol) / 10.F;
    saveVolume();
}

/// Store new Sound mode value into a EEPROM. Stored value size is 1byte
void Sound::saveMode() {
    eeprom_set_var(EEVAR_SOUND_MODE, variant8_ui8((uint8_t)eSoundMode));
}

/// Store new Sound VOLUME value into a EEPROM.
void Sound::saveVolume() {
    eeprom_set_var(EEVAR_SOUND_VOLUME, variant8_ui8((uint8_t)(varVolume * 10.F)));
}

/// [stopSound] is in this moment just for stopping infinitely repeating sound signal in LOUD & ASSIST mode
void Sound::stop() {
    frequency = 100.F;
    _duration = 0;
    duration = 0;
    repeat = 0;
    _delay = 0;
}

void Sound::_playSound(eSOUND_TYPE sound, const eSOUND_TYPE types[], const int repeats[], unsigned size) {
    for (unsigned i = 0; i < size; i++) {
        eSOUND_TYPE type = types[i];
        if (type == sound) {
            _sound(repeats[i], frequencies[(size_t)type], durations[(size_t)type], volumes[(size_t)type] /* , Sound::forced[type] */);
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
        _playSound(eSoundType, onceTypes, onceRepeats, t_size);
        break;
    case eSOUND_MODE::SILENT:
        t_size = sizeof(silentTypes) / sizeof(silentTypes[0]);
        _playSound(eSoundType, silentTypes, silentRepeats, t_size);
        break;
    case eSOUND_MODE::ASSIST:
        t_size = sizeof(assistTypes) / sizeof(assistTypes[0]);
        _playSound(eSoundType, assistTypes, assistRepeats, t_size);
        break;
    case eSOUND_MODE::LOUD:
    default:
        t_size = sizeof(loudTypes) / sizeof(loudTypes[0]);
        _playSound(eSoundType, loudTypes, loudRepeats, t_size);
        break;
    }
}

/// Generic [_sound] method with setting values and repeating logic
void Sound::_sound(int rep, float frq, uint32_t dur, float vol /*, bool forced*/) {
    /// if sound is already playing, then don't interrupt
    if ((repeat - 1 > 0 || repeat == -1) /*  && !forced */) {
        return;
    }

    /// store variables for timing method
    repeat = rep;
    frequency = frq;
    duration = dur;
    volume = (vol * varVolume) * 0.3F;

    /// end previous beep
    hwio_beeper_set_pwm(0, 0);
    nextRepeat();
}

/// Another repeat of sound signal. Just set live variable with duration of the beep and play it
void Sound::nextRepeat() {
    _duration = duration;
    _delay = 1;
    if (repeat > 1 || repeat == -1) {
        _delay = delay;
    }
    hwio_beeper_tone2(frequency, duration, volume);
}

/*!
 * Update method to control duration of sound signals and repeating count.
 * When variable [repeat] is -1, then repeating will be infinite until [stopSound] is called.
 */
void Sound::update1ms() {
    /// -- timing logic without osDelay for repeating Beep(s)
    _duration = _duration <= 0 ? 0 : _duration - 1;
    if (_duration <= 0) {
        if (--_delay <= 0) {
            repeat = repeat == -1 ? -1 : repeat - 1;
            if ((repeat != 0) || (repeat == -1)) {
                nextRepeat();
            }
        }
    }

    /// calling hwio update fnc
    hwio_update_1ms();
}
