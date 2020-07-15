#include "catch2/catch.hpp"

#include <iostream>
#include <assert.h>

#include "language_eeprom.hpp"
#include "../lang/translator.hpp"

TEST_CASE("LangEEPROM::setLanguage()", "[LangEEPROM]") {
    LangEEPROM::getInstance().setLanguage(Translations::MakeLangCode("en"));
}

TEST_CASE("LangEEPROM::getLanguage()", "[LangEEPROM]") {
}
