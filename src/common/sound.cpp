#include "sound.h"
#include "hwio.h"
#include "eeprom.h"
// #include "cmsis_os.h"

// -- Singleton class
Sound* Sound::getInstance(){
    static Sound s;
    if (!s._inited){ s.soundInit(); }
    return &s;
}

uint32_t Sound::_duration = 0;
uint32_t Sound::duration = 0;
uint8_t Sound::repeat = 0;
double Sound::frequency = 100.0;
double Sound::volume = 0.0125;

void Sound::soundInit(){
    eSoundMode = (eSOUND_MODE)eeprom_get_var(EEVAR_SOUND_MODE).ui8;
    if((uint8_t)eSoundMode == (uint8_t)eSOUND_MODE_NULL){
        this->setMode(eSOUND_MODE_DEFAULT);
    }
    _inited = true;
}

eSOUND_MODE Sound::getMode(){
    return eSoundMode;  
}

void Sound::setMode(eSOUND_MODE eSMode){
    eSoundMode = eSMode;
    this->saveMode();
}

void Sound::saveMode(){
    eeprom_set_var(EEVAR_SOUND_MODE, variant8_ui8((uint8_t)eSoundMode));
}

void Sound::stopSound(){
    _duration = 0;
}

void Sound::doSound(eSOUND_TYPE eSoundType){
    switch (eSoundMode){
        case eSOUND_MODE_ONCE:
            if(eSoundType == eSOUND_TYPE_ButtonEcho) { this->soundButtonEcho(1, 100.f); }
            if(eSoundType == eSOUND_TYPE_StandardPrompt) { this->soundStandardPrompt(1, 500.f); }
            if(eSoundType == eSOUND_TYPE_StandardAlert) { this->soundStandardAlert(1, 200.f); }
            break;
        case eSOUND_MODE_LOUD:
            if(eSoundType == eSOUND_TYPE_ButtonEcho) { this->soundButtonEcho(1, 100.0f); }
            if(eSoundType == eSOUND_TYPE_StandardPrompt) { this->soundStandardPrompt(5, 500.f); }
            if(eSoundType == eSOUND_TYPE_StandardAlert) { this->soundStandardAlert(3, 200.f); }
            break;
        case eSOUND_MODE_SILENT:
            if(eSoundType == eSOUND_TYPE_StandardAlert) { this->soundStandardAlert(1, 200.f); }
            break;
        case eSOUND_MODE_ASSIST:
            if(eSoundType == eSOUND_TYPE_ButtonEcho) { this->soundButtonEcho(1, 100.0f); }
            if(eSoundType == eSOUND_TYPE_StandardPrompt) { this->soundStandardPrompt(5, 500.f); }
            if(eSoundType == eSOUND_TYPE_StandardAlert) { this->soundStandardAlert(3, 200.f); }
            if(eSoundType == eSOUND_TYPE_EncoderMove) { this->soundEncoderMove(1, 50.f); }
            if(eSoundType == eSOUND_TYPE_BlindAlert) { this->soundBlindAlert(1, 100.f); }
            break;
        default:
            break;
    }
}

void Sound::soundButtonEcho(int rep, uint32_t del){
    float vol = (double)(0.01F * 0.125F);
    float frq = 100.0f;
    this->_sound(rep, frq, del, vol);
}

void Sound::soundStandardPrompt(int rep, uint32_t del){
    float vol = (double)(0.01F * 0.125F);
    float frq = 500.0f;
    this->_sound(rep, frq, del, vol);
}

void Sound::soundStandardAlert(int rep, uint32_t del){
    float vol = (double)(0.01F * 0.125F);
    float frq = 500.0f;
    this->_sound(rep, frq, del, vol);
}

void Sound::soundEncoderMove(int rep, uint32_t del){
    float vol = (double)(0.01F * 0.125F);
    float frq = 500.0f;
    this->_sound(rep, frq, del, vol);
}

void Sound::soundBlindAlert(int rep, uint32_t del){
    float vol = (double)(0.01F * 0.125F);
    float frq = 500.0f;
    this->_sound(rep, frq, del, vol);
}

void Sound::_sound(int rep, float frq, uint32_t del, float vol){
    repeat = rep;
    frequency = frq;
    duration = del;
    _duration = del;
    volume = vol;
    // uint8_t nI;
    hwio_beeper_set_pwm(0, 0); // -- end previous beep
    Sound::nextRepeat();

    // for (nI=0; nI<rep; nI++){
    //     hwio_beeper_tone2(frq, del, vol);
    //     // if (rep > 1) { osDelay(del); }
    // }
}

void Sound::nextRepeat(){
    _duration = duration;
    hwio_beeper_tone2(frequency, duration, volume);
}

void  Sound::soundUpdate1ms(){
    // -- timing logic without osDelay for repeating Beep(s) - viva la Kenshi
    if ((_duration) && (--_duration == 0)){
        if((repeat) && (--repeat != 0)){
            Sound::nextRepeat();
        }
    }
    // -- call hwio update
    hwio_update_1ms();
}
