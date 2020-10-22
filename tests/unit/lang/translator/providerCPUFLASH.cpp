#include "catch2/catch.hpp"

#include "translator.hpp"
#include "translation_provider_CPUFLASH.hpp"
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <deque>
#include <map>
#include <set>
#include "hash.hpp"
#include "unaccent.hpp"

using namespace std;

constexpr size_t maxStringBegins = 256;
constexpr size_t maxUtf8Raw = 16384;

/// just like the StringTableCS, but without const data - to be able to fill them during testing at runtime
struct StringTableCSTest {
    // this will get statically precomputed for each translation language separately
    static uint16_t stringCount, stringBytes;
    static uint16_t stringBeginsSize, utf8RawSize;
    static uint16_t stringBegins[maxStringBegins];
    // a piece of memory where the null-terminated strings are situated
    static uint8_t utf8Raw[maxUtf8Raw];

    static void Reset() {
        stringBeginsSize = 0;
        utf8RawSize = 0;
        fill(stringBegins, stringBegins + maxStringBegins, 0);
        fill(utf8Raw, utf8Raw + maxUtf8Raw, 0);
    }
};

uint16_t StringTableCSTest::stringBegins[maxStringBegins];
uint8_t StringTableCSTest::utf8Raw[maxUtf8Raw];
uint16_t StringTableCSTest::stringCount, StringTableCSTest::stringBytes;
uint16_t StringTableCSTest::stringBeginsSize, StringTableCSTest::utf8RawSize;

using CPUFLASHTranslationProviderCSTest = CPUFLASHTranslationProvider<StringTableCSTest>;

struct StringTableDETest {
    // this will get statically precomputed for each translation language separately
    static uint16_t stringCount, stringBytes;
    static uint16_t stringBegins[maxStringBegins];
    static uint16_t stringBeginsSize, utf8RawSize;
    // a piece of memory where the null-terminated strings are situated
    static uint8_t utf8Raw[maxUtf8Raw];

    static void Reset() {
        stringBeginsSize = 0;
        utf8RawSize = 0;
        fill(stringBegins, stringBegins + maxStringBegins, 0);
        fill(utf8Raw, utf8Raw + maxUtf8Raw, 0);
    }
};

uint16_t StringTableDETest::stringBegins[maxStringBegins];
uint8_t StringTableDETest::utf8Raw[maxUtf8Raw];
uint16_t StringTableDETest::stringCount, StringTableDETest::stringBytes;
uint16_t StringTableDETest::stringBeginsSize, StringTableDETest::utf8RawSize;

using CPUFLASHTranslationProviderDETest = CPUFLASHTranslationProvider<StringTableDETest>;

struct StringTableESTest {
    // this will get statically precomputed for each translation language separately
    static uint16_t stringCount, stringBytes;
    static uint16_t stringBeginsSize, utf8RawSize;
    static uint16_t stringBegins[maxStringBegins];
    // a piece of memory where the null-terminated strings are situated
    static uint8_t utf8Raw[maxUtf8Raw];

    static void Reset() {
        stringBeginsSize = 0;
        utf8RawSize = 0;
        fill(stringBegins, stringBegins + maxStringBegins, 0);
        fill(utf8Raw, utf8Raw + maxUtf8Raw, 0);
    }
};

uint16_t StringTableESTest::stringBegins[maxStringBegins];
uint8_t StringTableESTest::utf8Raw[maxUtf8Raw];
uint16_t StringTableESTest::stringCount, StringTableESTest::stringBytes;
uint16_t StringTableESTest::stringBeginsSize, StringTableESTest::utf8RawSize;

using CPUFLASHTranslationProviderESTest = CPUFLASHTranslationProvider<StringTableESTest>;

struct StringTableFRTest {
    // this will get statically precomputed for each translation language separately
    static uint16_t stringCount, stringBytes;
    static uint16_t stringBeginsSize, utf8RawSize;
    static uint16_t stringBegins[maxStringBegins];
    // a piece of memory where the null-terminated strings are situated
    static uint8_t utf8Raw[maxUtf8Raw];

    static void Reset() {
        stringBeginsSize = 0;
        utf8RawSize = 0;
        fill(stringBegins, stringBegins + maxStringBegins, 0);
        fill(utf8Raw, utf8Raw + maxUtf8Raw, 0);
    }
};

uint16_t StringTableFRTest::stringBegins[maxStringBegins];
uint8_t StringTableFRTest::utf8Raw[maxUtf8Raw];
uint16_t StringTableFRTest::stringCount, StringTableFRTest::stringBytes;
uint16_t StringTableFRTest::stringBeginsSize, StringTableFRTest::utf8RawSize;

using CPUFLASHTranslationProviderFRTest = CPUFLASHTranslationProvider<StringTableFRTest>;

struct StringTableITTest {
    // this will get statically precomputed for each translation language separately
    static uint16_t stringCount, stringBytes;
    static uint16_t stringBeginsSize, utf8RawSize;
    static uint16_t stringBegins[maxStringBegins];
    // a piece of memory where the null-terminated strings are situated
    static uint8_t utf8Raw[maxUtf8Raw];

    static void Reset() {
        stringBeginsSize = 0;
        utf8RawSize = 0;
        fill(stringBegins, stringBegins + maxStringBegins, 0);
        fill(utf8Raw, utf8Raw + maxUtf8Raw, 0);
    }
};

uint16_t StringTableITTest::stringBegins[maxStringBegins];
uint8_t StringTableITTest::utf8Raw[maxUtf8Raw];
uint16_t StringTableITTest::stringCount, StringTableITTest::stringBytes;
uint16_t StringTableITTest::stringBeginsSize, StringTableITTest::utf8RawSize;

using CPUFLASHTranslationProviderITTest = CPUFLASHTranslationProvider<StringTableITTest>;

struct StringTablePLTest {
    // this will get statically precomputed for each translation language separately
    static uint16_t stringCount, stringBytes;
    static uint16_t stringBeginsSize, utf8RawSize;
    static uint16_t stringBegins[maxStringBegins];
    // a piece of memory where the null-terminated strings are situated
    static uint8_t utf8Raw[maxUtf8Raw];

    static void Reset() {
        stringBeginsSize = 0;
        utf8RawSize = 0;
        fill(stringBegins, stringBegins + maxStringBegins, 0);
        fill(utf8Raw, utf8Raw + maxUtf8Raw, 0);
    }
};

uint16_t StringTablePLTest::stringBegins[maxStringBegins];
uint8_t StringTablePLTest::utf8Raw[maxUtf8Raw];
uint16_t StringTablePLTest::stringCount, StringTablePLTest::stringBytes;
uint16_t StringTablePLTest::stringBeginsSize, StringTablePLTest::utf8RawSize;

using CPUFLASHTranslationProviderPLTest = CPUFLASHTranslationProvider<StringTablePLTest>;

TEST_CASE("providerCPUFLASH::StringTableAt", "[translator]") {
    // simple test of several strings - setup first
    StringTableCSTest::Reset();

    StringTableCSTest::stringBegins[0] = 0;
    static const char str0[] = "first string";
    strcpy((char *)StringTableCSTest::utf8Raw, str0);

    StringTableCSTest::stringBegins[1] = sizeof(str0) + 1;
    static const char str1[] = "second long string";
    strcpy((char *)StringTableCSTest::utf8Raw + StringTableCSTest::stringBegins[1], str1);

    StringTableCSTest::stringBegins[2] = StringTableCSTest::stringBegins[1] + sizeof(str1) + 1;
    static const char str2[] = "příliš žluťoučký kůň";
    strcpy((char *)StringTableCSTest::utf8Raw + StringTableCSTest::stringBegins[2], str2);

    CPUFLASHTranslationProviderCSTest provider;

    // casting to const char * just to see the vars in the debugger like strings
    const char *s0 = (const char *)provider.StringTableAt(0);
    CHECK(!strcmp(s0, str0));

    const char *s1 = (const char *)provider.StringTableAt(1);
    CHECK(!strcmp(s1, str1));

    const char *s2 = (const char *)provider.StringTableAt(2);
    CHECK(!strcmp(s2, str2));
}

/// @returns number of bytes the strings require to store
pair<uint16_t, uint16_t> FillStringTable(const deque<string> &translatedStrings, uint16_t *stringBegins, uint8_t *utf8Raw) {
    uint8_t *utf8RawOrigin = utf8Raw;
    uint16_t *stringsBeginOrigin = stringBegins;
    for_each(translatedStrings.cbegin(), translatedStrings.cend(), [&](const string &s) {
        *stringBegins = utf8Raw - utf8RawOrigin;
        ++stringBegins;
        strcpy((char *)utf8Raw, s.c_str());
        utf8Raw += s.size();
        *utf8Raw = 0; // terminate the string
        ++utf8Raw;
    });
    return make_pair(stringBegins - stringsBeginOrigin, utf8Raw - utf8RawOrigin);
}

bool CompareStringViews(string_view_utf8 s, string_view_utf8 s2, set<unichar> &nonAsciiChars) {
    unichar c;
    while ((c = s.getUtf8Char()) != 0) {
        if (c > 128) {
            nonAsciiChars.insert(c); // just stats how many non-ASCII UTF-8 characters do we have for now
            const auto &cASCII = UnaccentTable::Utf8RemoveAccents(c);
            if (cASCII.key == 0xffff) {
                // this string wants a new non-ascii character
                std::cout << "xx\n";
            }
        }
        if (c != s2.getUtf8Char()) {
            return false;
        }
    }
    return true;
}

bool LoadTranslatedStringsFile(const char *fname, deque<string> *st) {
    ifstream f(fname);
    REQUIRE(f.is_open());
    do {
        string s;
        getline(f, s);
        PreprocessRawLineStrings(s);
        if (!s.empty()) {              // beware of empty strings
            st->emplace_back(move(s)); // make a copy of the string
        }
    } while (f.good());
    return true;
}

bool CheckAllTheStrings(const deque<string> &rawStringKeys, const deque<string> &translatedStrings,
    CPUFLASHTranslationProviderBase &provider, set<unichar> &nonAsciiChars) {
    // prepare a map for comparison
    map<string, string> stringControlMap;
    {
        auto rsi = rawStringKeys.cbegin();
        auto csi = translatedStrings.cbegin();
        for (; rsi != rawStringKeys.cend(); ++rsi, ++csi) {
            stringControlMap[*rsi] = *csi;
        }
    }

    //    { // problematic items:
    //        string sk("Cooldown");
    //        string_view_utf8 s = provider.GetText(sk.c_str());
    //        string_view_utf8 s2 = string_view_utf8::MakeRAM((const uint8_t *)stringControlMapCS[sk].c_str());
    //        CHECK(CompareStringViews(s, s2));
    //    }

    // now do the lookup
    // I really WANT C++14 in this case!
    for_each(stringControlMap.cbegin(), stringControlMap.cend(), [&](const auto &v) {
        const char *key = v.first.c_str();
        string_view_utf8 s = provider.GetText(key);
        // make the same interface over the translated string
        const char *value = v.second.c_str();
        string_view_utf8 s2 = string_view_utf8::MakeRAM((const uint8_t *)value);
        // now compare - that means iterating over both string views and making sure both return the same utf8 characters
        CHECK(CompareStringViews(s, s2, nonAsciiChars));
    });

    { // check for a non-existing string
        static const char nex[] = "NoN_ExIsTiNg:string";
        string_view_utf8 s = provider.GetText(nex);
        string_view_utf8 s2 = string_view_utf8::MakeRAM((const uint8_t *)nex);
        CHECK(CompareStringViews(s, s2, nonAsciiChars));
    }

    return true;
}

template <typename T>
void FillStringTable(const deque<string> &strings) {
    auto sizes = FillStringTable(strings, T::stringBegins, T::utf8Raw);
    T::stringBeginsSize = sizes.first;
    T::utf8RawSize = sizes.second;
}

/// Binary compare of providers' data - between the local one created in these unit tests (tstP)
/// and the one which was generated by the new python scripts and will be compiled into the firmware (compP)
template <typename T>
void CompareProviders(const T *tstP, const char *langCode) {
    Translations::Instance().ChangeLanguage(Translations::MakeLangCode(langCode));
    const CPUFLASHTranslationProviderBase *compP = dynamic_cast<const CPUFLASHTranslationProviderBase *>(Translations::Instance().CurrentProvider());
    REQUIRE(compP);
    const uint16_t *tstPSB = tstP->StringBegins();
    const uint8_t *tstPU8 = tstP->Utf8Raw();

    const uint16_t *compPSB = compP->StringBegins();
    const uint8_t *compPU8 = compP->Utf8Raw();

    // Now the tricky part - we need size of the arrays.
    // Since the arrays are expected to be of the same size and content,
    // we can abuse the testing one to provide the necessary size information
    REQUIRE(std::mismatch(tstPSB, tstPSB + T::RawData::stringBeginsSize, compPSB).first == tstPSB + T::RawData::stringBeginsSize);
    REQUIRE(std::mismatch(tstPU8, tstPU8 + T::RawData::utf8RawSize, compPU8).first == tstPU8 + T::RawData::utf8RawSize);
}

/// Ideally (not necessarily) run this test before the ComplexTest just to make sure the languages exist
TEST_CASE("providerCPUFLASH::Translations singleton", "[translator]") {
    REQUIRE(Translations::Instance().LangExists(Translations::MakeLangCode("cs")));
    REQUIRE(Translations::Instance().LangExists(Translations::MakeLangCode("de")));
    REQUIRE(Translations::Instance().LangExists(Translations::MakeLangCode("en")));
    REQUIRE(Translations::Instance().LangExists(Translations::MakeLangCode("es")));
    REQUIRE(Translations::Instance().LangExists(Translations::MakeLangCode("fr")));
    REQUIRE(Translations::Instance().LangExists(Translations::MakeLangCode("it")));
    REQUIRE(Translations::Instance().LangExists(Translations::MakeLangCode("pl")));
}

/// This is a complex test of the whole translation mechanism
/// We must prepare the search structures first and then lookup all the string keys
TEST_CASE("providerCPUFLASH::ComplexTest", "[translator]") {
    StringTableCSTest::Reset();

    // now we'll need more input files
    // cat Prusa-Firmware-Buddy_cs.po | grep msgstr | cut -b 8- | sed "s@\"@@g" > cs.txt
    // we already have the keys.txt, now we also need the translated texts
    // All that will be fed into the hash map (separate tests)
    // and the translated texts into the stringtable
    // After that, we may prove, that all the keys can be found correctly

    CPUFLASHTranslationProviderCSTest providerCS;
    CPUFLASHTranslationProviderDETest providerDE;
    CPUFLASHTranslationProviderESTest providerES;
    CPUFLASHTranslationProviderFRTest providerFR;
    CPUFLASHTranslationProviderITTest providerIT;
    CPUFLASHTranslationProviderPLTest providerPL;
    deque<string> rawStringKeys;
    FillHashTableCPUFLASHProvider(CPUFLASHTranslationProviderBase::hash_table, "keys.txt", rawStringKeys);

    // now do a similar thing for the translated strings
    deque<string> csStrings, deStrings, esStrings, frStrings, itStrings, plStrings;
    REQUIRE(LoadTranslatedStringsFile("cs.txt", &csStrings));
    REQUIRE(LoadTranslatedStringsFile("de.txt", &deStrings));
    REQUIRE(LoadTranslatedStringsFile("es.txt", &esStrings));
    REQUIRE(LoadTranslatedStringsFile("fr.txt", &frStrings));
    REQUIRE(LoadTranslatedStringsFile("it.txt", &itStrings));
    REQUIRE(LoadTranslatedStringsFile("pl.txt", &plStrings));

    // need to have at least the same amount of translations like the keys (normally there will be an exact number of them)
    REQUIRE(rawStringKeys.size() <= csStrings.size());
    REQUIRE(rawStringKeys.size() <= deStrings.size());
    REQUIRE(rawStringKeys.size() <= esStrings.size());
    REQUIRE(rawStringKeys.size() <= frStrings.size());
    REQUIRE(rawStringKeys.size() <= itStrings.size());
    REQUIRE(rawStringKeys.size() <= plStrings.size());

    // now make the string table from cs.txt
    FillStringTable<StringTableCSTest>(csStrings);
    FillStringTable<StringTableDETest>(deStrings);
    FillStringTable<StringTableESTest>(esStrings);
    FillStringTable<StringTableFRTest>(frStrings);
    FillStringTable<StringTableITTest>(itStrings);
    FillStringTable<StringTablePLTest>(plStrings);

    // prepare a map for comparison
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
    REQUIRE(CheckAllTheStrings(rawStringKeys, csStrings, providerCS, nonASCIICharacters));
    REQUIRE(CheckAllTheStrings(rawStringKeys, deStrings, providerDE, nonASCIICharacters));
    REQUIRE(CheckAllTheStrings(rawStringKeys, esStrings, providerES, nonASCIICharacters));
    REQUIRE(CheckAllTheStrings(rawStringKeys, frStrings, providerFR, nonASCIICharacters));
    REQUIRE(CheckAllTheStrings(rawStringKeys, itStrings, providerIT, nonASCIICharacters));
    REQUIRE(CheckAllTheStrings(rawStringKeys, plStrings, providerPL, nonASCIICharacters));

    CompareProviders(&providerCS, "cs");
    CompareProviders(&providerDE, "de");
    CompareProviders(&providerES, "es");
    CompareProviders(&providerFR, "fr");
    CompareProviders(&providerIT, "it");
    CompareProviders(&providerPL, "pl");

    // @@TODO Things to check
    // 1. One of the things that remains to be checked is the hash search table - from this test and from the compile time generated variant
    // This is not easy to do right now, because it is built in CPUFLASHTranslationProviderBase as a static member
    // 2. Check the content of generated non-ascii-chars - to see, if we have enough font bitmaps

    {
        for_each(nonASCIICharacters.begin(), nonASCIICharacters.end(), [/*&f, &fr*/](unichar c) {
            // with accents, we don't need the unaccent table anymore
            // but is important for character generation (newly added characters)
            // check, that we have this character in our temporary translation table
            const auto &cASCII = UnaccentTable::Utf8RemoveAccents(c);
            CHECK(cASCII.key != 0xffff);
        });
    }
}
