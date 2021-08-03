#include "catch2/catch.hpp"

#include "provider.h"
#include "translation_provider_FILE.hpp"

using namespace std;

TEST_CASE("providerFILE::Translations test", "[translator]") {
    deque<string> stringKeys;
    CHECK(LoadTranslatedStringsFile("Mofiles/keys.txt", &stringKeys));

    //initialize translation providers
    FILETranslationProvider providerCS("Mofiles/cs.mo");
    FILETranslationProvider providerDE("Mofiles/de.mo");
    FILETranslationProvider providerES("Mofiles/es.mo");
    FILETranslationProvider providerFR("Mofiles/fr.mo");
    FILETranslationProvider providerIT("Mofiles/it.mo");
    FILETranslationProvider providerPL("Mofiles/pl.mo");

    //load transtaled strings
    deque<string> csStrings, deStrings, esStrings, frStrings, itStrings, plStrings;
    REQUIRE(LoadTranslatedStringsFile("Mofiles/cs.txt", &csStrings));
    REQUIRE(LoadTranslatedStringsFile("Mofiles/de.txt", &deStrings));
    REQUIRE(LoadTranslatedStringsFile("Mofiles/es.txt", &esStrings));
    REQUIRE(LoadTranslatedStringsFile("Mofiles/fr.txt", &frStrings));
    REQUIRE(LoadTranslatedStringsFile("Mofiles/it.txt", &itStrings));
    REQUIRE(LoadTranslatedStringsFile("Mofiles/pl.txt", &plStrings));

    // need to have at least the same amount of translations like the keys (normally there will be an exact number of them)
    REQUIRE(stringKeys.size() <= csStrings.size());
    REQUIRE(stringKeys.size() <= deStrings.size());
    REQUIRE(stringKeys.size() <= esStrings.size());
    REQUIRE(stringKeys.size() <= frStrings.size());
    REQUIRE(stringKeys.size() <= itStrings.size());
    REQUIRE(stringKeys.size() <= plStrings.size());

    set<unichar> nonASCIICharacters;
    {
        // explicitly add characters from language names
        // Čeština, Español, Français
        static const uint8_t na[] = "Čšñç";
        string_view_utf8 nas = string_view_utf8::MakeRAM(na);
        unichar c;
        while ((c = nas.getUtf8Char()) != 0) {
            nonASCIICharacters.insert(c);
        }
    }
    providerCS.EnsureFile();
    providerCS.EnsureFile();
    providerES.EnsureFile();
    providerFR.EnsureFile();
    providerIT.EnsureFile();
    providerPL.EnsureFile();

    REQUIRE(CheckAllTheStrings(stringKeys, csStrings, providerCS, nonASCIICharacters, "cs"));
    REQUIRE(CheckAllTheStrings(stringKeys, deStrings, providerDE, nonASCIICharacters, "de"));
    REQUIRE(CheckAllTheStrings(stringKeys, esStrings, providerES, nonASCIICharacters, "es"));
    REQUIRE(CheckAllTheStrings(stringKeys, frStrings, providerFR, nonASCIICharacters, "fr"));
    REQUIRE(CheckAllTheStrings(stringKeys, itStrings, providerIT, nonASCIICharacters, "it"));
    REQUIRE(CheckAllTheStrings(stringKeys, plStrings, providerPL, nonASCIICharacters, "pl"));

    CAPTURE(stringKeys.size());
    size_t i = 0;
}
