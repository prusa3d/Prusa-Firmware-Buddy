#include "catch2/catch.hpp"

#include "provider.h"
#include "translation_provider_FILE.hpp"

using namespace std;

TEST_CASE("providerFILE::Translations test", "[translator]") {
    deque<string> stringKeys;
    CHECK(LoadTranslatedStringsFile("MO/keys.txt", &stringKeys));

    // initialize translation providers
    FILETranslationProvider providerCS("MO/cs.mo");
    FILETranslationProvider providerDE("MO/de.mo");
    FILETranslationProvider providerES("MO/es.mo");
    FILETranslationProvider providerFR("MO/fr.mo");
    FILETranslationProvider providerIT("MO/it.mo");
    FILETranslationProvider providerPL("MO/pl.mo");
    // FILETranslationProvider providerPL("MO/ja.mo");

    // load transtaled strings
    deque<string> csStrings, deStrings, esStrings, frStrings, itStrings, plStrings;
    REQUIRE(LoadTranslatedStringsFile("MO/cs.txt", &csStrings));
    REQUIRE(LoadTranslatedStringsFile("MO/de.txt", &deStrings));
    REQUIRE(LoadTranslatedStringsFile("MO/es.txt", &esStrings));
    REQUIRE(LoadTranslatedStringsFile("MO/fr.txt", &frStrings));
    REQUIRE(LoadTranslatedStringsFile("MO/it.txt", &itStrings));
    REQUIRE(LoadTranslatedStringsFile("MO/pl.txt", &plStrings));
    // REQUIRE(LoadTranslatedStringsFile("MO/ja.txt", &jaStrings));

    // need to have at least the same amount of translations like the keys (normally there will be an exact number of them)
    REQUIRE(stringKeys.size() <= csStrings.size());
    REQUIRE(stringKeys.size() <= deStrings.size());
    REQUIRE(stringKeys.size() <= esStrings.size());
    REQUIRE(stringKeys.size() <= frStrings.size());
    REQUIRE(stringKeys.size() <= itStrings.size());
    REQUIRE(stringKeys.size() <= plStrings.size());
    // REQUIRE(stringKeys.size() <= jaStrings.size());

    set<unichar> nonASCIICharacters;
    {
        // explicitly add characters from language names
        // Čeština, Español, Français, Japanese
        // static const uint8_t na[] = "Čšñçニホンゴ";
        static const uint8_t na[] = "Čšñç";
        string_view_utf8 nas = string_view_utf8::MakeRAM(na);
        unichar c;
        while ((c = nas.getUtf8Char()) != 0) {
            nonASCIICharacters.insert(c);
        }
    }
    REQUIRE(providerCS.EnsureFile());
    REQUIRE(providerCS.EnsureFile());
    REQUIRE(providerES.EnsureFile());
    REQUIRE(providerFR.EnsureFile());
    REQUIRE(providerIT.EnsureFile());
    REQUIRE(providerPL.EnsureFile());
    // REQUIRE(providerJA.EnsureFile());

    REQUIRE(CheckAllTheStrings(stringKeys, csStrings, providerCS, nonASCIICharacters, "cs"));
    REQUIRE(CheckAllTheStrings(stringKeys, deStrings, providerDE, nonASCIICharacters, "de"));
    REQUIRE(CheckAllTheStrings(stringKeys, esStrings, providerES, nonASCIICharacters, "es"));
    REQUIRE(CheckAllTheStrings(stringKeys, frStrings, providerFR, nonASCIICharacters, "fr"));
    REQUIRE(CheckAllTheStrings(stringKeys, itStrings, providerIT, nonASCIICharacters, "it"));
    REQUIRE(CheckAllTheStrings(stringKeys, plStrings, providerPL, nonASCIICharacters, "pl"));
    // REQUIRE(CheckAllTheStrings(stringKeys, jaStrings, providerJA, nonASCIICharacters, "ja"));

    CAPTURE(stringKeys.size());
}

TEST_CASE("providerFILE::bad files test", "[translator]") {
    FILETranslationProvider nonExistingFile("nOnExIsTiNg.mo");
    FILETranslationProvider shortFile("MO/short.mo");
    FILETranslationProvider badMagic("MO/magic.mo");
    FILETranslationProvider bigEnd("MO/bigEnd.mo");

    REQUIRE(!nonExistingFile.EnsureFile());
    REQUIRE(shortFile.EnsureFile());
    REQUIRE(!badMagic.EnsureFile());
    REQUIRE(!bigEnd.EnsureFile());
    static const char *key = "Language";
    set<unichar> chars;
    // the file is short and should return key string
    REQUIRE(CompareStringViews(shortFile.GetText(key), string_view_utf8::MakeRAM((uint8_t *)key), chars, "ts"));
}
