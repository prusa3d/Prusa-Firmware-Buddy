#include "sound.hpp"
#include "hwio.h"
#include <config_store/store_instance.hpp>

struct SoundPattern {
    int8_t repeat; /// signals repeats - how many times will sound signals repeat (-1 is infinite)
    int16_t delay; /// delays for repeat sounds (ms)
    int16_t duration; /// durations for sound modes
};

static constexpr SoundPattern SILENCE = { 0, 0, 0 };

struct SoundSettings {
    SoundPattern once;
    SoundPattern loud;
    SoundPattern silent;
    SoundPattern assist;
    float frequency; /// frequency of signals in ms
    float volume; /// volumes of signals in ms
    bool forced; /// forced types of sounds - mainly for ERROR sounds. Ignores volume settings.

    const SoundPattern &pattern(eSOUND_MODE mode) const {
        switch (mode) {
        case eSOUND_MODE::ONCE:
            return once;
        case eSOUND_MODE::LOUD:
            return loud;
        case eSOUND_MODE::SILENT:
            return silent;
        case eSOUND_MODE::ASSIST:
            return assist;
        default:
            bsod("sound pattern");
        }
    }
};

struct AllSoundSettings {
    SoundSettings button_echo;
    SoundSettings standard_prompt;
    SoundSettings standard_alert;
    SoundSettings critical_alert;
    SoundSettings encoder_move;
    SoundSettings blind_alert;
    SoundSettings start;
    SoundSettings single_beep;
    SoundSettings waiting_beep;

    const SoundSettings &settings(eSOUND_TYPE type) const {
        switch (type) {
        case eSOUND_TYPE::ButtonEcho:
            return button_echo;
        case eSOUND_TYPE::StandardPrompt:
            return standard_prompt;
        case eSOUND_TYPE::StandardAlert:
            return standard_alert;
        case eSOUND_TYPE::CriticalAlert:
            return critical_alert;
        case eSOUND_TYPE::EncoderMove:
            return encoder_move;
        case eSOUND_TYPE::BlindAlert:
            return blind_alert;
        case eSOUND_TYPE::Start:
            return start;
        case eSOUND_TYPE::SingleBeep:
            return single_beep;
        case eSOUND_TYPE::WaitingBeep:
            return waiting_beep;
        case eSOUND_TYPE::SingleBeepAlwaysLoud:
            return single_beep;
        default:
            bsod("sound pattern");
        }
    }
};

static constexpr AllSoundSettings all_sound_settings = {
    .button_echo = {
        .once = { 1, 1, 100 },
        .loud = { 1, 1, 100 },
        .silent = SILENCE,
        .assist = { 1, 1, 100 },
        .frequency = 900.F,
        .volume = Sound::volumeInit,
        .forced = false,
    },
    .standard_prompt = {
        .once = { 1, 1, 500 },
        .loud = { -1, 1, 500 },
        .silent = SILENCE,
        .assist = { -1, 1, 500 },
        .frequency = 600.F,
        .volume = Sound::volumeInit,
        .forced = false,
    },
    .standard_alert = {
        .once = SILENCE,
        .loud = { 3, 1, 200 },
        .silent = { 1, 1, 200 },
        .assist = { 3, 1, 200 },
        .frequency = 950.F,
        .volume = Sound::volumeInit,
        .forced = false,
    },
    .critical_alert = {
        .once = { -1, 250, 500 },
        .loud = { -1, 250, 500 },
        .silent = { -1, 250, 500 },
        .assist = { -1, 250, 500 },
        .frequency = 999.F,
        .volume = Sound::volumeInit,
        .forced = true,
    },
    .encoder_move = {
        .once = SILENCE,
        .loud = SILENCE,
        .silent = SILENCE,
        .assist = { 1, 1, 10 },
        .frequency = 800.F,
        .volume = 0.175F,
        .forced = false,
    },
    .blind_alert = {
        .once = SILENCE,
        .loud = SILENCE,
        .silent = SILENCE,
        .assist = { 1, 1, 50 },
        .frequency = 500.F,
        .volume = 0.175F,
        .forced = false,
    },
    .start = {
        .once = { 1, 1, 100 },
        .loud = { 1, 1, 100 },
        .silent = { 1, 1, 100 },
        .assist = { 1, 1, 100 },
        .frequency = 999.F,
        .volume = Sound::volumeInit,
        .forced = false,
    },
    .single_beep = {
        .once = { 1, 1, 800 },
        .loud = { 1, 1, 800 },
        .silent = SILENCE,
        .assist = { 1, 1, 800 },
        .frequency = 950.F,
        .volume = Sound::volumeInit,
        .forced = false,
    },
    .waiting_beep = {
        .once = { 1, 1, 800 },
        .loud = { -1, 2000, 100 },
        .silent = SILENCE,
        .assist = { -1, 2000, 100 },
        .frequency = 800.F,
        .volume = Sound::volumeInit,
        .forced = false,
    },
};

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
    if (eeprom_mode <= eSOUND_MODE::_last) {
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

void Sound::_playSound(eSOUND_TYPE type, eSOUND_MODE mode) {
    const SoundSettings &settings = all_sound_settings.settings(type);
    const SoundPattern &pattern = settings.pattern(mode);
    if (pattern.duration) {
        _sound(pattern.repeat, settings.frequency, pattern.duration, pattern.delay, settings.volume, settings.forced);
    }
}

/*!
 * Generag [play] method with sound type parameter where dependetly on set mode is played.
 * Every mode handle just his own signal types.
 */
void Sound::play(eSOUND_TYPE eSoundType) {
    eSOUND_MODE mode = eSoundMode;

    if (eSoundType == eSOUND_TYPE::CriticalAlert || eSoundType == eSOUND_TYPE::SingleBeepAlwaysLoud) {
        mode = eSOUND_MODE::LOUD;
    }

    switch (mode) {
    case eSOUND_MODE::ONCE:
    case eSOUND_MODE::SILENT:
    case eSOUND_MODE::ASSIST:
    case eSOUND_MODE::LOUD:
        _playSound(eSoundType, mode);
        break;
    default:
        _playSound(eSoundType, eSOUND_MODE::LOUD);
        break;
    }
}

/// Generic [_sound] method with setting values and repeating logic
void Sound::_sound(int rep, float frq, int16_t dur, int16_t del, [[maybe_unused]] float vol, bool f) {
    /// forced non-repeat sounds - can be played when another
    /// repeating sound is playing
    float tmpVol;

#if BOARD_IS_BUDDY()
    if (varVolume > 1) {
        tmpVol = 1.F;
    } else {
        tmpVol = f ? 0.3F : (vol * varVolume) * 0.3F;
    }
#else
    tmpVol = f ? volumeInit : varVolume;
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
#ifdef _DEBUG
        /// for BSOD debugging
        if (eSoundMode == eSOUND_MODE::DEBUG) {
            volume = 0;
        }
#endif

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
#if BOARD_IS_BUDDY()
    return displayed_volume == 11 ? displayed_volume : displayed_volume / 10.F;
#else
    return displayed_volume == 0 ? 0 : 1.51F - displayed_volume / 2.F;
#endif
}

uint8_t Sound::displayed_volume(float real_volume) {
#if BOARD_IS_BUDDY()
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
