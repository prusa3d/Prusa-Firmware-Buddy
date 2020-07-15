#include "catch2/catch.hpp"

#include <iostream>
#include <assert.h>
#include <array>

#include "language_eeprom.hpp"
#include "../lang/translator.hpp"

TEST_CASE("Languages::getInstance() - GET / SET", "[LangEEPROM]") {
    char langChar[2] = "";
    uint16_t langCode = 0;

    SECTION("LANG - EN") {
        langChar[0] = 'e';
        langChar[1] = 'n';
        langCode = Translations::MakeLangCode(langChar);
        LangEEPROM::getInstance().setLanguage(langCode);

        std::array<char, 2> al = LangEEPROM::getInstance().getLanguageChar();

        CHECK((al[0] == langChar[0] && al[1] == langChar[1]));
    }

    SECTION("LANG - CS") {
        langChar[0] = 'c';
        langChar[1] = 's';
        langCode = Translations::MakeLangCode(langChar);
        LangEEPROM::getInstance().setLanguage(langCode);

        std::array<char, 2> al = LangEEPROM::getInstance().getLanguageChar();

        CHECK((al[0] == langChar[0] && al[1] == langChar[1]));
    }

    SECTION("LANG - DE") {
        langChar[0] = 'd';
        langChar[1] = 'e';
        langCode = Translations::MakeLangCode(langChar);
        LangEEPROM::getInstance().setLanguage(langCode);

        std::array<char, 2> al = LangEEPROM::getInstance().getLanguageChar();

        CHECK((al[0] == langChar[0] && al[1] == langChar[1]));
    }

    SECTION("LANG - ES") {
        langChar[0] = 'e';
        langChar[1] = 's';
        langCode = Translations::MakeLangCode(langChar);
        LangEEPROM::getInstance().setLanguage(langCode);

        std::array<char, 2> al = LangEEPROM::getInstance().getLanguageChar();

        CHECK((al[0] == langChar[0] && al[1] == langChar[1]));
    }

    SECTION("LANG - FR") {
        langChar[0] = 'f';
        langChar[1] = 'r';
        langCode = Translations::MakeLangCode(langChar);
        LangEEPROM::getInstance().setLanguage(langCode);

        std::array<char, 2> al = LangEEPROM::getInstance().getLanguageChar();

        CHECK((al[0] == langChar[0] && al[1] == langChar[1]));
    }

    SECTION("LANG - IT") {
        langChar[0] = 'i';
        langChar[1] = 't';
        langCode = Translations::MakeLangCode(langChar);
        LangEEPROM::getInstance().setLanguage(langCode);

        std::array<char, 2> al = LangEEPROM::getInstance().getLanguageChar();

        CHECK((al[0] == langChar[0] && al[1] == langChar[1]));
    }

    SECTION("LANG - PL") {
        langChar[0] = 'p';
        langChar[1] = 'l';
        langCode = Translations::MakeLangCode(langChar);
        LangEEPROM::getInstance().setLanguage(langCode);

        std::array<char, 2> al = LangEEPROM::getInstance().getLanguageChar();

        CHECK((al[0] == langChar[0] && al[1] == langChar[1]));
    }
}
