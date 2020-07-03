#include "catch2/catch.hpp"

#include "translator.hpp"
#include "translation_provider_CPUFLASH.hpp"
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <deque>

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

/// This is a complex test of the whole translation mechanism
/// We must prepare the search structures first and then lookup all the string keys
TEST_CASE("providerCPUFLASH::ComplexTest", "[translator]") {
    StringTableCSTest::Reset();

    // budem potrebovat nekolik vstupnich fajlu
    // cat Prusa-Firmware-Buddy_cs.po | grep msgstr | cut -b 8- | sed "s@\"@@g" > cs.txt
    // keys uz mame, ted k tomu stejne serazene jednotlive preklady
    // tim se nakrmi vyhledavaci hashmapa (to uz je otestovano)
    // a pro kazdej jazyk se naplni i stringtable
    // pak lze proverit, ze se vsechny stringy spravne najdou
}
