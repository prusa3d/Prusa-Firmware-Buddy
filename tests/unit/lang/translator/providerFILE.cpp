#include "catch2/catch.hpp"

#include "translator.hpp"
#include "translation_provider_FILE.hpp"
#include "hash.hpp"
#include "unaccent.hpp"
#include <iostream>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <deque>
#include <set>
#include <map>

using namespace std;

bool CompareStringView(string_view_utf8 s, string_view_utf8 s2, set<unichar> &nonAsciiChars, const char *langCode) {
    unichar c;
    while ((c = s.getUtf8Char()) != 0) {
        if (c > 128) {
            nonAsciiChars.insert(c); // just stats how many non-ASCII UTF-8 characters do we have for now
            const auto &cASCII = UnaccentTable::Utf8RemoveAccents(c);

            if (cASCII.key == 0xffff) {
                // this string wants a new non-ascii character - force fail the whole test immediately
                // When this happens, one must either add the character into unaccent.cpp, if the character is meaningfull
                // Or kick the translator person to stop copying BS formatting characters from MS Word into Phraseapp
                // Typical situation: U+202A -> e2 80 aa -> LEFT-TO-RIGHT EMBEDDING
                char tmp[1024];
                s2.rewind();
                s2.copyToRAM(tmp, 1024);
                INFO("Language=" << langCode << " : string='" << tmp << "': needs an unknown non-ASCII character ord=0x" << std::hex << c);
                REQUIRE(cASCII.key != 0xffff);
                return false;
            }
        }
        if (c != s2.getUtf8Char()) {
            return false;
        }
    }
    return true;
}

bool CheckAllTheStrings(const deque<string> &rawStringKeys, const deque<string> &translatedStrings,
    FILEtranslationProvider &provider, set<unichar> &nonAsciiChars, const char *langCode) {
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
        string outString;
        size_t numOfChars = s.computeNumUtf8CharsAndRewind();
        for (size_t i = 0; i < numOfChars; ++i) {
            outString.append(1, s.getUtf8Char());
        }
        s.rewind();

        // make the same interface over the translated string
        const char *value = v.second.c_str();
        string_view_utf8 s2 = string_view_utf8::MakeRAM((const uint8_t *)value);
        UNSCOPED_INFO("key is " << v.first);
        UNSCOPED_INFO("final is " << v.second);
        UNSCOPED_INFO("res is " << outString);
        // now compare - that means iterating over both string views and making sure both return the same utf8 characters
        CHECK(CompareStringView(s, s2, nonAsciiChars, langCode));
    });

    { // check for a non-existing string
        static const char nex[] = "NoN_ExIsTiNg:string";
        string_view_utf8 s = provider.GetText(nex);
        string_view_utf8 s2 = string_view_utf8::MakeRAM((const uint8_t *)nex);
        CHECK(CompareStringView(s, s2, nonAsciiChars, langCode));
    }

    return true;
}

bool loadStrings(const char *fname, deque<string> &keys) {
    {
        ifstream f(fname);
        if (!f.is_open())
            return false;
        uint16_t index = 0;
        do {
            string s;
            getline(f, s);
            INFO("test" << s)
            PreprocessRawLineStrings(s);
            if (!s.empty()) {      // beware of empty strings
                keys.push_back(s); // make a copy of the string
                REQUIRE(keys.back() == s);
                ++index;
            }
        } while (f.good());
    }
    return true;
}

TEST_CASE("providerFILE::Translations test", "[translator]") {
    deque<string> stringKeys;
    deque<string> translatedStrings;
    CHECK(loadStrings("keys.txt", stringKeys));
    FILEtranslationProvider fileProvider("cs.mo");
    CHECK(loadStrings("cs.txt", translatedStrings));

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
    fileProvider.OpenFile();
    CheckAllTheStrings(stringKeys, translatedStrings, fileProvider, nonASCIICharacters, "cs");

    CAPTURE(stringKeys.size());
    size_t i = 0;
}
