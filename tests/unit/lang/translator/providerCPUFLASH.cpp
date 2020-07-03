#include "catch2/catch.hpp"

#include "translator.hpp"
#include "translation_provider_CPUFLASH.hpp"
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <deque>
#include <map>
#include "hash.hpp"

using namespace std;

/// just like the StringTableCS, but without const data - to be able to fill them during testing at runtime
struct StringTableCSTest {
    // this will get statically precomputed for each translation language separately
    static uint16_t stringBegins[256];
    // a piece of memory where the null-terminated strings are situated
    static uint8_t utf8Raw[4096];

    static void Reset() {
        fill(stringBegins, stringBegins + 256, 0);
        fill(utf8Raw, utf8Raw + 4096, 0);
    }
};

uint16_t StringTableCSTest::stringBegins[256];
uint8_t StringTableCSTest::utf8Raw[4096];

using CPUFLASHTranslationProviderCSTest = CPUFLASHTranslationProvider<StringTableCSTest>;

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

void FillStringTable(const deque<string> &translatedStrings, uint16_t *stringBegins, uint8_t *utf8Raw) {
    uint8_t *utf8RawOrigin = utf8Raw;
    for_each(translatedStrings.cbegin(), translatedStrings.cend(), [&](const string &s) {
        *stringBegins = utf8Raw - utf8RawOrigin;
        ++stringBegins;
        strcpy((char *)utf8Raw, s.c_str());
        utf8Raw += s.size();
        *utf8Raw = 0; // terminate the string
        ++utf8Raw;
    });
}

bool CompareStringViews(string_view_utf8 s, string_view_utf8 s2) {
    unichar c;
    while ((c = s.getUtf8Char()) != 0) {
        if (c != s2.getUtf8Char()) {
            return false;
        }
    }
    return true;
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

    CPUFLASHTranslationProviderCSTest provider;
    deque<string> rawStringKeys;
    FillHashTableCPUFLASHProvider(CPUFLASHTranslationProviderBase::hash_table, "keys.txt", rawStringKeys);

    // now do a similar thing for the translated strings
    deque<string> csStrings;
    {
        ifstream f("cs.txt");
        REQUIRE(f.is_open());
        do {
            string s;
            getline(f, s);
            if (!s.empty()) {                    // beware of empty strings
                csStrings.emplace_back(move(s)); // make a copy of the string
            }
        } while (f.good());
    }

    // need to have at least the same amount of translations like the keys (normally there will be an exact number of them)
    REQUIRE(rawStringKeys.size() <= csStrings.size());

    // now make the string table from cs.txt
    FillStringTable(csStrings, StringTableCSTest::stringBegins, StringTableCSTest::utf8Raw);

    // prepare a map for comparison
    map<string, string> stringControlMap;
    {
        auto rsi = rawStringKeys.cbegin();
        auto csi = csStrings.cbegin();
        for (; rsi != rawStringKeys.cend(); ++rsi, ++csi) {
            stringControlMap[*rsi] = *csi;
        }
    }

    { // problematic items:
        string sk("Cooldown");
        string_view_utf8 s = provider.GetText(sk.c_str());
        string_view_utf8 s2 = string_view_utf8::MakeRAM((const uint8_t *)stringControlMap[sk].c_str());
        CHECK(CompareStringViews(s, s2));
    }

    // now do the lookup
    // I really WANT C++14 in this case!
    for_each(stringControlMap.cbegin(), stringControlMap.cend(), [&](const auto &v) {
        const char *key = v.first.c_str();
        string_view_utf8 s = provider.GetText(key);
        // make the same interface over the translated string
        const char *value = v.second.c_str();
        string_view_utf8 s2 = string_view_utf8::MakeRAM((const uint8_t *)value);
        // now compare - that means iterating over both string views and making sure both return the same utf8 characters
        CHECK(CompareStringViews(s, s2));
    });
}
