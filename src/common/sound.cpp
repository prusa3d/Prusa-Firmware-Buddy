#include "sound_C_wrapper.h"
#include "sound.h"
#include "hwio.h"
#include "eeprom.h"

// -- Sound signals implementation
// Simple sound implementation supporting few sound modes and having different sound types.
// [Sound] is updated every 1ms with tim14 tick from [appmain.cpp] for meassured durations of sound signals for non-blocking GUI.
// Beeper is controled over [hwio_a3ides_2209_02.c] functions for beeper.
Sound::Sound() {
    _duration = 0;     // live variable used for meassure
    duration = 0;      // added variable to set _duration for repeating
    repeat = 0;        // how many times is sound played
    frequency = 100.f; // frequency of sound signal (0-1000)
    volume = 0.50;     // volume of sound signal (0-1)

    this->init();
}

// Inicialization of Singleton Class needs to be AFTER eeprom inicialization.
// [soundInit] is getting stored EEPROM value of his sound mode.
// [soundInit] sets global variable [SOUND_INIT] for safe update method([soundUpdate1ms]) because tim14 tick update method is called before [eeprom.c] is initialized.
void Sound::init() {
    eSoundMode = (eSOUND_MODE)eeprom_get_var(EEVAR_SOUND_MODE).ui8;
    if ((uint8_t)eSoundMode == (uint8_t)eSOUND_MODE_NULL) {
        this->setMode(eSOUND_MODE_DEFAULT);
    }
    // GLOBAL FLAG set on demand when first sound method is called
    SOUND_INIT = 1;
}

eSOUND_MODE Sound::getMode() {
    return eSoundMode;
}

void Sound::setMode(eSOUND_MODE eSMode) {
    eSoundMode = eSMode;
    this->saveMode();
}

// Store new Sound mode value into a EEPROM. Stored value size is 1byte
void Sound::saveMode() {
    eeprom_set_var(EEVAR_SOUND_MODE, variant8_ui8((uint8_t)eSoundMode));
}

// [stopSound] is in this moment just for stopping infinitely repeating sound signal in LOUD & ASSIST mode
void Sound::stop() {
    frequency = 100.f;
    _duration = 0;
    duration = 0;
    repeat = 0;
}

// Generag [play] method with sound type parameter where dependetly on set mode is played.
// Every mode handle just his own signal types.
void Sound::play(eSOUND_TYPE eSoundType) {
    switch (eSoundMode) {
    case eSOUND_MODE_ONCE:
        if (eSoundType == eSOUND_TYPE_Start) {
            this->soundStart(1, 100.f);
        }
        if (eSoundType == eSOUND_TYPE_ButtonEcho) {
            this->soundButtonEcho(1, 100.f);
        }
        if (eSoundType == eSOUND_TYPE_StandardPrompt) {
            this->soundStandardPrompt(1, 500.f);
        }
        if (eSoundType == eSOUND_TYPE_StandardAlert) {
            this->soundStandardAlert(1, 200.f);
        }
        if (eSoundType == eSOUND_TYPE_CriticalAlert) {
            this->soundCriticalAlert(-1, 500.f);
        }
        break;
    case eSOUND_MODE_LOUD:
        if (eSoundType == eSOUND_TYPE_Start) {
            this->soundStart(1, 100.f);
        }
        if (eSoundType == eSOUND_TYPE_ButtonEcho) {
            this->soundButtonEcho(1, 100.0f);
        }
        if (eSoundType == eSOUND_TYPE_StandardPrompt) {
            this->soundStandardPrompt(-1, 500.f);
        }
        if (eSoundType == eSOUND_TYPE_StandardAlert) {
            this->soundStandardAlert(3, 200.f);
        }
        if (eSoundType == eSOUND_TYPE_CriticalAlert) {
            this->soundCriticalAlert(-1, 500.f);
        }
        break;
    case eSOUND_MODE_SILENT:
        if (eSoundType == eSOUND_TYPE_Start) {
            this->soundStart(1, 100.f);
        }
        if (eSoundType == eSOUND_TYPE_StandardAlert) {
            this->soundStandardAlert(1, 200.f);
        }
        if (eSoundType == eSOUND_TYPE_CriticalAlert) {
            this->soundCriticalAlert(-1, 500.f);
        }
        break;
    case eSOUND_MODE_ASSIST:
        if (eSoundType == eSOUND_TYPE_Start) {
            this->soundStart(1, 100.f);
        }
        if (eSoundType == eSOUND_TYPE_ButtonEcho) {
            this->soundButtonEcho(1, 100.0f);
        }
        if (eSoundType == eSOUND_TYPE_StandardPrompt) {
            this->soundStandardPrompt(-1, 500.f);
        }
        if (eSoundType == eSOUND_TYPE_StandardAlert) {
            this->soundStandardAlert(3, 200.f);
        }
        if (eSoundType == eSOUND_TYPE_EncoderMove) {
            this->soundEncoderMove(1, 50.f);
        }
        if (eSoundType == eSOUND_TYPE_BlindAlert) {
            this->soundBlindAlert(1, 100.f);
        }
        if (eSoundType == eSOUND_TYPE_CriticalAlert) {
            this->soundCriticalAlert(-1, 500.f);
        }
        break;
    default:
        break;
    }
}

// Sound signal played once just after boot
void Sound::soundStart(int rep, uint32_t del) {
    float frq = 999.0f;
    this->_sound(rep, frq, del, volume);
}

// Sound signal for encoder button click
void Sound::soundButtonEcho(int rep, uint32_t del) {
    float frq = 900.0f;
    this->_sound(rep, frq, del, volume);
}

// Sound signal for user needed input on prompt screens (filament runout, etc.)
void Sound::soundStandardPrompt(int rep, uint32_t del) {
    float frq = 600.0f;
    this->_sound(rep, frq, del, volume);
}

// Souns signal for errors, bsod, and others Alert type's events
void Sound::soundStandardAlert(int rep, uint32_t del) {
    float frq = 950.0f;
    this->_sound(rep, frq, del, volume);
}

// Souns signal for errors, bsod, and others Alert type's events
void Sound::soundCriticalAlert(int rep, uint32_t del) {
    float frq = 999.0f;
    this->_sound(rep, frq, del, 1.0);
}

// Sound signal every time when encoder nove
void Sound::soundEncoderMove(int rep, uint32_t del) {
    float frq = 800.0f;
    this->_sound(rep, frq, del, 0.25);
}

// Sound signal for signaling start and end of the menu or items selecting on screen
void Sound::soundBlindAlert(int rep, uint32_t del) {
    float frq = 500.0f;
    this->_sound(rep, frq, del, 0.25);
}

// Generic [_sound[ method with setting values and repeating logic
void Sound::_sound(int rep, float frq, uint32_t del, float vol) {
    // if sound is already playing, then don't interrupt
    if (repeat - 1 > 0 || repeat == -1) {
        return;
    }

    // store variables for timing method
    repeat = rep;
    frequency = frq;
    duration = del;
    volume = vol;

    // end previous beep
    hwio_beeper_set_pwm(0, 0);
    this->nextRepeat();
}

// Another repeat of sound signal. Just set live variable with duration of the beep and play it
void Sound::nextRepeat() {
    _duration = duration;
    hwio_beeper_tone2(frequency, duration, volume);
}

// Update method to control duration of sound signals and repeating count.
// When variable [repeat] is -1, then repeating will be infinite until [stopSound] is called.
void Sound::update1ms() {
    // -- timing logic without osDelay for repeating Beep(s)
    if ((_duration) && (--_duration <= 0)) {
        if (((repeat) && (--repeat != 0)) || (repeat == -1)) {
            this->nextRepeat();
        }
    }
    // calling hwio update fnc
    hwio_update_1ms();
}
