
#include "language_eeprom.hpp"
#include "../lang/translator.hpp"

/*!
 * Setter & getter class to store language code into a EEPROM
 * Constructor will check if language code is not set, then it will set
 * a default value.
 * If it's set, then Translator is called to start translating.
 */
LangEEPROM::LangEEPROM()
    : _language(0) {
#ifndef LANGEEPROM_UNITTEST
    uint16_t _language = static_cast<uint16_t>(eeprom_get_var(EEVAR_LANGUAGE).ui16);
#else
    uint16_t _language = Translations::MakeLangCode("en");
#endif
    if (_language == static_cast<uint16_t>(0xffff)) {
        setLanguage(Translations::MakeLangCode("en"));
    } else {
#ifndef LANGEEPROM_UNITTEST
        Translations::Instance().ChangeLanguage(_language);
#endif
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

#ifndef LANGEEPROM_UNITTEST
    Translations::Instance().ChangeLanguage(_language);
#endif
}

/// save new language code into a EEPROM
void LangEEPROM::saveLanguage() {
#ifndef LANGEEPROM_UNITTEST
    eeprom_set_var(EEVAR_LANGUAGE, variant8_ui16((uint16_t)_language));
#endif
}

/// return set language code in uint16_t
uint16_t LangEEPROM::getLanguage() {
    return _language;
}

/// return set language code in char[2]
std::array<char, 2> LangEEPROM::getLanguageChar() {
    char ch1 = _language & 0xFF;
    char ch2 = _language >> 8;

    std::array<char, 2> charCode;

    charCode[0] = ch1;
    charCode[1] = ch2;
    return charCode;
}
