#include "sound.h"
#include "hwio.h"
#include "eeprom.h"
#include "cmsis_os.h"

// -- Singleton class
Sound* Sound::getInstance(){
    static Sound s;
    if (!s._inited){ s.soundInit(); }
    return &s;
}

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
    uint8_t nI;
    hwio_beeper_set_pwm(0, 0);
    for (nI=0; nI<rep; nI++){
        hwio_beeper_tone2(frq, del, vol);
        if (rep > 1) { osDelay(del); }
    }
}

void  Sound::soundUpdate1ms(){
    if ((_del) && (--_del == 0)){
        /* code */
    }
    
    hwio_update_1ms();
}
