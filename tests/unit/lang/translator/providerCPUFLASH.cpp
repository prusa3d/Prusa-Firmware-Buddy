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
#include "rundir.h"

using namespace std;

constexpr size_t maxStringBegins = 256;
constexpr size_t maxUtf8Raw = 16384;

/// just like the StringTableCS, but without const data - to be able to fill them during testing at runtime
struct StringTableCSTest {
    // this will get statically precomputed for each translation language separately
    static uint16_t stringCount, stringBytes;
    static uint16_t stringBegins[maxStringBegins];
    // a piece of memory where the null-terminated strings are situated
    static uint8_t utf8Raw[maxUtf8Raw];

    static void Reset() {
        fill(stringBegins, stringBegins + maxStringBegins, 0);
        fill(utf8Raw, utf8Raw + maxUtf8Raw, 0);
    }
};

uint16_t StringTableCSTest::stringBegins[maxStringBegins];
uint8_t StringTableCSTest::utf8Raw[maxUtf8Raw];
uint16_t StringTableCSTest::stringCount, StringTableCSTest::stringBytes;

using CPUFLASHTranslationProviderCSTest = CPUFLASHTranslationProvider<StringTableCSTest>;

struct StringTableDETest {
    // this will get statically precomputed for each translation language separately
    static uint16_t stringCount, stringBytes;
    static uint16_t stringBegins[maxStringBegins];
    // a piece of memory where the null-terminated strings are situated
    static uint8_t utf8Raw[maxUtf8Raw];

    static void Reset() {
        fill(stringBegins, stringBegins + maxStringBegins, 0);
        fill(utf8Raw, utf8Raw + maxUtf8Raw, 0);
    }
};

uint16_t StringTableDETest::stringBegins[maxStringBegins];
uint8_t StringTableDETest::utf8Raw[maxUtf8Raw];
uint16_t StringTableDETest::stringCount, StringTableDETest::stringBytes;

using CPUFLASHTranslationProviderDETest = CPUFLASHTranslationProvider<StringTableDETest>;

struct StringTableESTest {
    // this will get statically precomputed for each translation language separately
    static uint16_t stringCount, stringBytes;
    static uint16_t stringBegins[maxStringBegins];
    // a piece of memory where the null-terminated strings are situated
    static uint8_t utf8Raw[maxUtf8Raw];

    static void Reset() {
        fill(stringBegins, stringBegins + maxStringBegins, 0);
        fill(utf8Raw, utf8Raw + maxUtf8Raw, 0);
    }
};

uint16_t StringTableESTest::stringBegins[maxStringBegins];
uint8_t StringTableESTest::utf8Raw[maxUtf8Raw];
uint16_t StringTableESTest::stringCount, StringTableESTest::stringBytes;

using CPUFLASHTranslationProviderESTest = CPUFLASHTranslationProvider<StringTableESTest>;

struct StringTableFRTest {
    // this will get statically precomputed for each translation language separately
    static uint16_t stringCount, stringBytes;
    static uint16_t stringBegins[maxStringBegins];
    // a piece of memory where the null-terminated strings are situated
    static uint8_t utf8Raw[maxUtf8Raw];

    static void Reset() {
        fill(stringBegins, stringBegins + maxStringBegins, 0);
        fill(utf8Raw, utf8Raw + maxUtf8Raw, 0);
    }
};

uint16_t StringTableFRTest::stringBegins[maxStringBegins];
uint8_t StringTableFRTest::utf8Raw[maxUtf8Raw];
uint16_t StringTableFRTest::stringCount, StringTableFRTest::stringBytes;

using CPUFLASHTranslationProviderFRTest = CPUFLASHTranslationProvider<StringTableFRTest>;

struct StringTableITTest {
    // this will get statically precomputed for each translation language separately
    static uint16_t stringCount, stringBytes;
    static uint16_t stringBegins[maxStringBegins];
    // a piece of memory where the null-terminated strings are situated
    static uint8_t utf8Raw[maxUtf8Raw];

    static void Reset() {
        fill(stringBegins, stringBegins + maxStringBegins, 0);
        fill(utf8Raw, utf8Raw + maxUtf8Raw, 0);
    }
};

uint16_t StringTableITTest::stringBegins[maxStringBegins];
uint8_t StringTableITTest::utf8Raw[maxUtf8Raw];
uint16_t StringTableITTest::stringCount, StringTableITTest::stringBytes;

using CPUFLASHTranslationProviderITTest = CPUFLASHTranslationProvider<StringTableITTest>;

struct StringTablePLTest {
    // this will get statically precomputed for each translation language separately
    static uint16_t stringCount, stringBytes;
    static uint16_t stringBegins[maxStringBegins];
    // a piece of memory where the null-terminated strings are situated
    static uint8_t utf8Raw[maxUtf8Raw];

    static void Reset() {
        fill(stringBegins, stringBegins + maxStringBegins, 0);
        fill(utf8Raw, utf8Raw + maxUtf8Raw, 0);
    }
};

uint16_t StringTablePLTest::stringBegins[maxStringBegins];
uint8_t StringTablePLTest::utf8Raw[maxUtf8Raw];
uint16_t StringTablePLTest::stringCount, StringTablePLTest::stringBytes;

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
        if (c > 128)
            nonAsciiChars.insert(c); // just stats how many non-ASCII UTF-8 characters do we have for now
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
        // must convert the '\n' into \xa here
        FindAndReplaceAll(s, string("\\n"), string("\xa"));
        // 0x7f symbol for degrees is a similar case
        FindAndReplaceAll(s, string("\\177"), string("\177"));
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

void SaveArray(const char *strData, uint16_t strDataSize, const char *type, const char *langCode) {
    ofstream f(string(RELATIVE_FROM_RUNDIR "strings.") + type + langCode);
    f.write(strData, strDataSize);
}

template <typename T, typename Tfirst, typename Tsecond>
void SaveHashIPP(const T *p, size_t size, const char *fname, Tfirst first, Tsecond second) {
    ofstream f(fname);
    f << "{\n"
      << hex;
    for_each(p, p + size, [&](const T &i) {
        f << "{ 0x" << first(i) << "U, 0x" << second(i) << "U },\n";
    });
    f << "};\n";
}

template <typename T>
void SaveArray4CPP(const T *p, size_t size, const char *fname, const char *decl) {
    ofstream f(fname);
    f << decl << " = {\n"
      << hex;
    for_each(p, p + size, [&](const T &i) {
        f << "0x" << i << ", ";
    });
    f << "};\n";
}

template <typename T>
void FillAndSaveStringTable(const deque<string> &strings, const char *langCode) {
    auto ssize = FillStringTable(strings, T::stringBegins, T::utf8Raw);
    T::stringCount = ssize.first;
    T::stringBytes = ssize.second;
    SaveArray((const char *)T::utf8Raw, T::stringBytes, "table.", langCode);
    SaveArray((const char *)T::stringBegins, T::stringCount * sizeof(uint16_t), "index.", langCode);

    string upcaseLangCode(langCode);
    std::transform(upcaseLangCode.begin(), upcaseLangCode.end(), upcaseLangCode.begin(), ::toupper);
    SaveArray4CPP(T::stringBegins, T::stringCount, (string(RELATIVE_FROM_RUNDIR "stringBegins.") + langCode + ".hpp").c_str(),
        (string("const uint16_t StringTable") + upcaseLangCode + "::stringBegins[]").c_str());
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
    FillHashTableCPUFLASHProvider(CPUFLASHTranslationProviderBase::hash_table, RELATIVE_FROM_RUNDIR "keys.txt", rawStringKeys);
    const auto &ht = CPUFLASHTranslationProviderBase::hash_table;
    SaveArray((const char *)ht.hash_table, sizeof(ht.hash_table), RELATIVE_FROM_RUNDIR "hash_table", "");
    SaveArray((const char *)ht.stringRecArray, sizeof(ht.stringRecArray), RELATIVE_FROM_RUNDIR "hash_buckets", "");
    SaveHashIPP(
        ht.hash_table, ht.Buckets(), RELATIVE_FROM_RUNDIR "hash_table_buckets.ipp",
        [](const CPUFLASHTranslationProviderBase::SHashTable::BucketRange &b) { return b.begin; },
        [](const CPUFLASHTranslationProviderBase::SHashTable::BucketRange &b) { return b.end; });
    SaveHashIPP(
        ht.stringRecArray, ht.MaxStrings(), RELATIVE_FROM_RUNDIR "hash_table_string_indices.ipp",
        [](const CPUFLASHTranslationProviderBase::SHashTable::BucketItem &b) { return b.firstLetters; },
        [](const CPUFLASHTranslationProviderBase::SHashTable::BucketItem &b) { return b.stringIndex; });

    // now do a similar thing for the translated strings
    deque<string> csStrings, deStrings, esStrings, frStrings, itStrings, plStrings; // don't have the Spanish translation yet
    REQUIRE(LoadTranslatedStringsFile(RELATIVE_FROM_RUNDIR "cs.txt", &csStrings));
    REQUIRE(LoadTranslatedStringsFile(RELATIVE_FROM_RUNDIR "de.txt", &deStrings));
    REQUIRE(LoadTranslatedStringsFile(RELATIVE_FROM_RUNDIR "es.txt", &esStrings));
    REQUIRE(LoadTranslatedStringsFile(RELATIVE_FROM_RUNDIR "fr.txt", &frStrings));
    REQUIRE(LoadTranslatedStringsFile(RELATIVE_FROM_RUNDIR "it.txt", &itStrings));
    REQUIRE(LoadTranslatedStringsFile(RELATIVE_FROM_RUNDIR "pl.txt", &plStrings));

    // need to have at least the same amount of translations like the keys (normally there will be an exact number of them)
    REQUIRE(rawStringKeys.size() <= csStrings.size());
    REQUIRE(rawStringKeys.size() <= deStrings.size());
    REQUIRE(rawStringKeys.size() <= esStrings.size());
    REQUIRE(rawStringKeys.size() <= frStrings.size());
    REQUIRE(rawStringKeys.size() <= itStrings.size());
    REQUIRE(rawStringKeys.size() <= plStrings.size());

    // now make the string table from cs.txt
    FillAndSaveStringTable<StringTableCSTest>(csStrings, "cs");
    FillAndSaveStringTable<StringTableDETest>(deStrings, "de");
    FillAndSaveStringTable<StringTableESTest>(esStrings, "es");
    FillAndSaveStringTable<StringTableFRTest>(frStrings, "fr");
    FillAndSaveStringTable<StringTableITTest>(itStrings, "it");
    FillAndSaveStringTable<StringTablePLTest>(plStrings, "pl");

    // prepare a map for comparison
    set<unichar> nonASCIICharacters;
    REQUIRE(CheckAllTheStrings(rawStringKeys, csStrings, providerCS, nonASCIICharacters));
    REQUIRE(CheckAllTheStrings(rawStringKeys, deStrings, providerDE, nonASCIICharacters));
    REQUIRE(CheckAllTheStrings(rawStringKeys, esStrings, providerES, nonASCIICharacters));
    REQUIRE(CheckAllTheStrings(rawStringKeys, frStrings, providerFR, nonASCIICharacters));
    REQUIRE(CheckAllTheStrings(rawStringKeys, itStrings, providerIT, nonASCIICharacters));
    REQUIRE(CheckAllTheStrings(rawStringKeys, plStrings, providerPL, nonASCIICharacters));

    {
        // cout << "Non-ASCII characters = " << nonASCIICharacters.size() << endl;
        // Dump detected non-ASCII characters to a file - note, I don't have the correct utf-8 represetation here
        // ... can be probably added later
        // In our case we have only 2-byte utf chars so far (cs, de, es, fr, it, pl)
        ofstream fr(RELATIVE_FROM_RUNDIR "non-ascii-chars.raw");
        ofstream f(RELATIVE_FROM_RUNDIR "non-ascii-chars.txt");
        for_each(nonASCIICharacters.begin(), nonASCIICharacters.end(), [&f, &fr](unichar c) {
            fr.write((const char *)&c, sizeof(unichar));
            uint8_t uc[4] = "   ";
            uc[0] = 0xc0 | (c >> 6);
            uc[1] = 0x80 | (c & 0x3f);
            f.write((const char *)uc, 3);

            // with accents, we don't need the unaccent table anymore
            //            // check, that we have this character in our temporary translation table
            //            const auto &cASCII = UnaccentTable::Utf8RemoveAccents(c);
            //            CHECK(cASCII.key != 0xffff);
        });
    }
}

TEST_CASE("providerCPUFLASH::Translations singleton", "[translator]") {
    REQUIRE(Translations::Instance().LangExists(Translations::MakeLangCode("cs")));
    REQUIRE(Translations::Instance().LangExists(Translations::MakeLangCode("de")));
    REQUIRE(Translations::Instance().LangExists(Translations::MakeLangCode("en")));
    REQUIRE(Translations::Instance().LangExists(Translations::MakeLangCode("es")));
    REQUIRE(Translations::Instance().LangExists(Translations::MakeLangCode("fr")));
    REQUIRE(Translations::Instance().LangExists(Translations::MakeLangCode("it")));
    REQUIRE(Translations::Instance().LangExists(Translations::MakeLangCode("pl")));
}
