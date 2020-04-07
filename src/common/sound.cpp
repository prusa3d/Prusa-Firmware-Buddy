#include "sound.hpp"
#include "hwio.h"
#include "eeprom.h"

// -- C API
// extern "C" void Sound_SetMode(eSOUND_MODE eSMode){
//     Sound::getInstance()->setMode(eSMode);
// }
// extern "C" void Sound_DoSound(eSOUND_TYPE eSoundType){
//     Sound::getInstance()->doSound(eSoundType);
// }

// -- Singleton class
Sound* Sound::m_pInstance = nullptr;

Sound* Sound::getInstance(){
    if(m_pInstance == nullptr){
        m_pInstance = new Sound();
        m_pInstance->soundInit();
    }
    return m_pInstance;
}

void Sound::soundInit(){
    eSoundMode = (eSOUND_MODE)eeprom_read_byte((uint8_t*)EEPROM_SOUND_MODE);
    if(eSoundMode == (uint8_t)eSOUND_MODE_NULL){
        this->setMode(eSOUND_MODE_DEFAULT);
    }
}

void Sound::setMode(eSOUND_MODE eSMode){
    eSoundMode = eSMode;
    this->saveMode();
}

void Sound::saveMode(){
    eeprom_update_byte((uint8_t*)EEPROM_SOUND_MODE,(uint8_t)eSoundMode);
}

void Sound::doSound(eSOUND_TYPE eSoundType){
    switch (eSoundMode){
        case eSOUND_MODE_ONCE:
            if(eSoundType == eSOUND_TYPE_ButtonEcho) { this->soundButtonEcho(1, 100.0f); }
            // if(eSoundType == eSOUND_TYPE_StandardPrompt) { this->soundStandardPrompt(); }
            // if(eSoundType == eSOUND_TYPE_StandardAlert) { this->soundStandardAlert(); }
            break;
        case eSOUND_MODE_LOUD:
            if(eSoundType == eSOUND_TYPE_ButtonEcho) { this->soundButtonEcho(1, 100.0f); }
            // if(eSoundType == eSOUND_TYPE_StandardPrompt) { this->soundStandardPrompt(); }
            // if(eSoundType == eSOUND_TYPE_StandardAlert) { this->soundStandardAlert(); }
            break;
        case eSOUND_MODE_SILENT:
            // if(eSoundType == eSOUND_TYPE_ButtonEcho) { this->soundButtonEcho(); }
            // if(eSoundType == eSOUND_TYPE_StandardPrompt) { this->soundStandardPrompt(); }
            // if(eSoundType == eSOUND_TYPE_StandardAlert) { this->soundStandardAlert(); }
            break;
        case eSOUND_MODE_ASSIST:
            // if(eSoundType == eSOUND_TYPE_ButtonEcho) { this->soundButtonEcho(); }
            // if(eSoundType == eSOUND_TYPE_StandardPrompt) { this->soundStandardPrompt(); }
            // if(eSoundType == eSOUND_TYPE_StandardAlert) { this->soundStandardAlert(); }
            break;
        default:
            break;
    }
}

void Sound::soundButtonEcho(int rep, uint32_t del){
    float vol = (double)(0.01F * 0.125F);
    float frq = 200.0f;
    this->_sound(rep, frq, del, vol);
}

void Sound::_sound(int rep, float frq, uint32_t del, float vol){
    uint8_t nI;
    for (nI=0; nI<rep; nI++){
        hwio_beeper_tone2(frq, del, vol);
    }
}
