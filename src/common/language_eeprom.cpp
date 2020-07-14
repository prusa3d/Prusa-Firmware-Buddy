
#include "language_eeprom.hpp"
#include "../lang/translator.hpp"

/*!
 * Setter & getter class to store language code into a EEPROM
 * Constructor will check if language code is not set, then it will set
 * a default value.
 * If it's set, then Translator is called to start translating.
 */
LangEEPROM::LangEEPROM() : _language(0) {
    uint16_t _language = static_cast<uint16_t>(eeprom_get_var(EEVAR_LANGUAGE).ui16);
    if (_language == static_cast<uint16_t>(0xffff)) {
        setLanguage(Translations::MakeLangCode("cs"));
    } else {
        Translations::Instance().ChangeLanguage(_language);
    }
}

/*!
 * Set new active language by it's code (char[2] -> uint16_t)
 * Then new code is saved into a EEPROM and Translator is called to start
 * translating strings
 */
void LangEEPROM::setLanguage(uint16_t lang) {
    _language = lang;
    saveLanguage();

    Translations::Instance().ChangeLanguage(_language);
}

/// save new language code into a EEPROM
void LangEEPROM::saveLanguage() {
    eeprom_set_var(EEVAR_LANGUAGE, variant8_ui16((uint16_t)_language));
}

/// return set language code in uint16_t
uint16_t LangEEPROM::getLanguage() {
    return _language;
}

/// return set language code in char[2]
const char * LangEEPROM::getLanguageChar() {
    return reinterpret_cast<const char*>(_language);
}
