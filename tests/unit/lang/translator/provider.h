#pragma once

#include "translator.hpp"
#include "translation_provider_CPUFLASH.hpp"
#include "hash.hpp"
#include <cstdio>
#include <deque>
#include <set>
#include <map>
#include "translation_provider_FILE.hpp"
#include "translator.hpp"

using namespace std;

bool CompareStringViews(string_view_utf8 s, string_view_utf8 s2, set<unichar> &nonAsciiChars, const char *langCode);
bool CheckAllTheStrings(const deque<string> &rawStringKeys, const deque<string> &translatedStrings,
    ITranslationProvider &provider, set<unichar> &nonAsciiChars, const char *langCode);
bool LoadTranslatedStringsFile(const char *fname, deque<string> *st);
