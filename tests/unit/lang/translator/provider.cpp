
#include <catch2/catch_test_macros.hpp>
#include "provider.h"
#include "translation_provider_FILE.hpp"
#include <iostream>
#include <font_data/font_data.hpp>
#include <fstream>
#include <stdio.h>

bool CompareStringViews(string_view_utf8 s, string_view_utf8 s2, const char *langCode) {
    unichar c;
    size_t i = 0;
    StringReaderUtf8 r(s);
    StringReaderUtf8 r2(s2);
    while ((c = r.getUtf8Char()) != 0) {
        i++;
        if (c > 128) {
            // any "full" font works here
            const auto &probe = font_data::regular_9x16_full;
            if (!probe.contains(c)) {
                // this string wants a new non-ascii character - force fail the whole test immediately
                // When this happens, one must either add the character into unaccent.cpp, if the character is meaningfull
                // Or kick the translator person to stop copying BS formatting characters from MS Word into Phraseapp
                // Typical situation: U+202A -> e2 80 aa -> LEFT-TO-RIGHT EMBEDDING
                char tmp[1024];
                s2.copyBytesToRAM(tmp, 1024);
                INFO("Language=" << langCode << " : string='" << tmp << "': needs an unknown non-ASCII character ord=0x" << std::hex << c);
                return false;
            }
        }

        unichar x = r2.getUtf8Char();
        if (c != x) {
            UNSCOPED_INFO("failed at: " << c);
            UNSCOPED_INFO("failed at: " << x);
            UNSCOPED_INFO("index on: " << i);
            return false;
        }
    }
    return true;
};

bool CheckAllTheStrings(const deque<string> &rawStringKeys, const deque<string> &translatedStrings,
    ITranslationProvider &provider, const char *langCode) {
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
    //        string_view_utf8 s2 = string_view_utf8::MakeRAM(stringControlMapCS[sk].c_str());
    //        CHECK(CompareStringViews(s, s2));
    //    }

    // now do the lookup
    // I really WANT C++14 in this case!
    for_each(stringControlMap.cbegin(), stringControlMap.cend(), [&](const auto &v) {
        const char *key = v.first.c_str();
        const string_view_utf8 s = provider.GetText(key);
        string outString;
        size_t numOfChars = s.computeNumUtf8Chars();

        {
            StringReaderUtf8 s2(s);
            for (size_t i = 0; i < numOfChars; ++i) {
                outString.append(1, s2.getUtf8Char());
            }
        }

        // make the same interface over the translated string
        const char *value = v.second.c_str();
        string_view_utf8 s2 = string_view_utf8::MakeRAM(value);
        UNSCOPED_INFO("key is " << v.first);
        UNSCOPED_INFO("fin is " << v.second);
        UNSCOPED_INFO("res is " << outString);
        // now compare - that means iterating over both string views and making sure both return the same utf8 characters
        CHECK(CompareStringViews(s, s2, langCode));
    });

    { // check for a non-existing string
        static const char nex[] = "NoN_ExIsTiNg:string";
        string_view_utf8 s = provider.GetText(nex);
        string_view_utf8 s2 = string_view_utf8::MakeRAM(nex);
        CHECK(CompareStringViews(s, s2, langCode));
    }

    return true;
};

bool LoadTranslatedStringsFile(const char *fname, deque<string> *st) {
    ifstream f(fname);
    REQUIRE(f.is_open());
    do {
        string s;
        getline(f, s);
        PreprocessRawLineStrings(s);
        if (!s.empty()) { // beware of empty strings
            st->emplace_back(move(s)); // make a copy of the string
        }
    } while (f.good());
    return true;
}
