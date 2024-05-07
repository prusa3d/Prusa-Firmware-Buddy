#include "catch2/catch.hpp"

#include <iostream>
#include <stdio.h>
#include "translator.hpp"
#include "string_view_utf8.hpp"

using namespace std;

unichar UniChar(const char *ss) {
    const uint8_t *s = reinterpret_cast<const uint8_t *>(ss);
    unichar ord = *s++;
    if (!UTF8_IS_NONASCII(ord)) {
        return ord;
    }
    ord &= 0x7F;
    for (unichar mask = 0x40; ord & mask; mask >>= 1) {
        ord &= ~mask;
    }
    while (UTF8_IS_CONT(*s)) {
        ord = (ord << 6) | (*s++ & 0x3F);
    }
    return ord;
}

bool TheYellowHorse(string_view_utf8 sf) {
    StringReaderUtf8 reader(sf);
    REQUIRE(reader.getUtf8Char() == UniChar("p"));
    REQUIRE(reader.getUtf8Char() == UniChar("ř"));
    REQUIRE(reader.getUtf8Char() == UniChar("í"));
    REQUIRE(reader.getUtf8Char() == UniChar("l"));
    REQUIRE(reader.getUtf8Char() == UniChar("i"));
    REQUIRE(reader.getUtf8Char() == UniChar("š"));
    REQUIRE(reader.getUtf8Char() == UniChar(" "));
    REQUIRE(reader.getUtf8Char() == UniChar("ž"));
    REQUIRE(reader.getUtf8Char() == UniChar("l"));
    REQUIRE(reader.getUtf8Char() == UniChar("u"));
    REQUIRE(reader.getUtf8Char() == UniChar("ť"));
    REQUIRE(reader.getUtf8Char() == UniChar("o"));
    REQUIRE(reader.getUtf8Char() == UniChar("u"));
    REQUIRE(reader.getUtf8Char() == UniChar("č"));
    REQUIRE(reader.getUtf8Char() == UniChar("k"));
    REQUIRE(reader.getUtf8Char() == UniChar("ý"));
    REQUIRE(reader.getUtf8Char() == UniChar(" "));
    REQUIRE(reader.getUtf8Char() == UniChar("k"));
    REQUIRE(reader.getUtf8Char() == UniChar("ů"));
    REQUIRE(reader.getUtf8Char() == UniChar("ň"));
    REQUIRE(reader.getUtf8Char() == UniChar(" "));
    REQUIRE(reader.getUtf8Char() == UniChar("ú"));
    REQUIRE(reader.getUtf8Char() == UniChar("p"));
    REQUIRE(reader.getUtf8Char() == UniChar("ě"));
    REQUIRE(reader.getUtf8Char() == UniChar("l"));
    REQUIRE(reader.getUtf8Char() == UniChar(" "));
    REQUIRE(reader.getUtf8Char() == UniChar("ď"));
    REQUIRE(reader.getUtf8Char() == UniChar("á"));
    REQUIRE(reader.getUtf8Char() == UniChar("b"));
    REQUIRE(reader.getUtf8Char() == UniChar("e"));
    REQUIRE(reader.getUtf8Char() == UniChar("l"));
    REQUIRE(reader.getUtf8Char() == UniChar("s"));
    REQUIRE(reader.getUtf8Char() == UniChar("k"));
    REQUIRE(reader.getUtf8Char() == UniChar("é"));
    REQUIRE(reader.getUtf8Char() == UniChar(" "));
    REQUIRE(reader.getUtf8Char() == UniChar("ó"));
    REQUIRE(reader.getUtf8Char() == UniChar("d"));
    REQUIRE(reader.getUtf8Char() == UniChar("y"));
    REQUIRE(reader.getUtf8Char() == UniChar(" "));
    REQUIRE(reader.getUtf8Char() == UniChar(":"));
    REQUIRE(reader.getUtf8Char() == UniChar(" "));
    REQUIRE(reader.getUtf8Char() == UniChar("P"));
    REQUIRE(reader.getUtf8Char() == UniChar("Ř"));
    REQUIRE(reader.getUtf8Char() == UniChar("Í"));
    REQUIRE(reader.getUtf8Char() == UniChar("L"));
    REQUIRE(reader.getUtf8Char() == UniChar("I"));
    REQUIRE(reader.getUtf8Char() == UniChar("Š"));
    REQUIRE(reader.getUtf8Char() == UniChar(" "));
    REQUIRE(reader.getUtf8Char() == UniChar("Ž"));
    REQUIRE(reader.getUtf8Char() == UniChar("L"));
    REQUIRE(reader.getUtf8Char() == UniChar("U"));
    REQUIRE(reader.getUtf8Char() == UniChar("Ť"));
    REQUIRE(reader.getUtf8Char() == UniChar("O"));
    REQUIRE(reader.getUtf8Char() == UniChar("U"));
    REQUIRE(reader.getUtf8Char() == UniChar("Č"));
    REQUIRE(reader.getUtf8Char() == UniChar("K"));
    REQUIRE(reader.getUtf8Char() == UniChar("Ý"));
    REQUIRE(reader.getUtf8Char() == UniChar(" "));
    REQUIRE(reader.getUtf8Char() == UniChar("K"));
    REQUIRE(reader.getUtf8Char() == UniChar("Ů"));
    REQUIRE(reader.getUtf8Char() == UniChar("Ň"));
    REQUIRE(reader.getUtf8Char() == UniChar(" "));
    REQUIRE(reader.getUtf8Char() == UniChar("Ú"));
    REQUIRE(reader.getUtf8Char() == UniChar("P"));
    REQUIRE(reader.getUtf8Char() == UniChar("Ě"));
    REQUIRE(reader.getUtf8Char() == UniChar("L"));
    REQUIRE(reader.getUtf8Char() == UniChar(" "));
    REQUIRE(reader.getUtf8Char() == UniChar("Ď"));
    REQUIRE(reader.getUtf8Char() == UniChar("Á"));
    REQUIRE(reader.getUtf8Char() == UniChar("B"));
    REQUIRE(reader.getUtf8Char() == UniChar("E"));
    REQUIRE(reader.getUtf8Char() == UniChar("L"));
    REQUIRE(reader.getUtf8Char() == UniChar("S"));
    REQUIRE(reader.getUtf8Char() == UniChar("K"));
    REQUIRE(reader.getUtf8Char() == UniChar("É"));
    REQUIRE(reader.getUtf8Char() == UniChar(" "));
    REQUIRE(reader.getUtf8Char() == UniChar("Ó"));
    REQUIRE(reader.getUtf8Char() == UniChar("D"));
    REQUIRE(reader.getUtf8Char() == UniChar("Y"));
    REQUIRE(reader.getUtf8Char() == 0);
    return true;
}

TEST_CASE("string_view_utf8::CreateFromRAM test", "[string_view_utf8]") {
    static const uint8_t utf8str[] = "příliš žluťoučký kůň úpěl ďábelské ódy : PŘÍLIŠ ŽLUŤOUČKÝ KŮŇ ÚPĚL ĎÁBELSKÉ ÓDY";
    string_view_utf8 sf = string_view_utf8::MakeRAM(utf8str);
    REQUIRE(TheYellowHorse(sf));
}

TEST_CASE("string_view_utf8::CreateFromCPUFLASH test", "[string_view_utf8]") {
    static const uint8_t utf8str[] = "příliš žluťoučký kůň úpěl ďábelské ódy : PŘÍLIŠ ŽLUŤOUČKÝ KŮŇ ÚPĚL ĎÁBELSKÉ ÓDY";
    string_view_utf8 sf = string_view_utf8::MakeCPUFLASH(utf8str);
    REQUIRE(TheYellowHorse(sf));
}

TEST_CASE("string_view_utf8::CreateFromFILE test", "[string_view_utf8]") {
    static const char fname[] = "zluty_kun.bin";
    FILE *f = fopen(fname, "rb"); // beware! the file must end with \0!
    REQUIRE(f); // without it it makes no sense to continue this test

    string_view_utf8 sf = string_view_utf8::MakeFILE(f, 0);
    // and now the fun begins - the string view shall return utf8 characters (deliberately typed into the file)
    REQUIRE(TheYellowHorse(sf));
    fclose(f);
}

bool Cooldown(string_view_utf8 sf) {
    StringReaderUtf8 reader(sf);
    REQUIRE(reader.getUtf8Char() == UniChar("O"));
    REQUIRE(reader.getUtf8Char() == UniChar("c"));
    REQUIRE(reader.getUtf8Char() == UniChar("h"));
    REQUIRE(reader.getUtf8Char() == UniChar("l"));
    REQUIRE(reader.getUtf8Char() == UniChar("a"));
    REQUIRE(reader.getUtf8Char() == UniChar("z"));
    REQUIRE(reader.getUtf8Char() == UniChar("e"));
    REQUIRE(reader.getUtf8Char() == UniChar("n"));
    REQUIRE(reader.getUtf8Char() == UniChar("í"));
    REQUIRE(reader.getUtf8Char() == 0);
    return true;
}

TEST_CASE("string_view_utf8::Cooldown test", "[string_view_utf8]") {
    static const uint8_t utf8str[] = "Ochlazení";
    string_view_utf8 sf = string_view_utf8::MakeRAM(utf8str);
    REQUIRE(Cooldown(sf));
}

bool Filament(string_view_utf8 sf) {
    StringReaderUtf8 reader(sf);
    REQUIRE(reader.getUtf8Char() == UniChar("F"));
    REQUIRE(reader.getUtf8Char() == UniChar("i"));
    REQUIRE(reader.getUtf8Char() == UniChar("l"));
    REQUIRE(reader.getUtf8Char() == UniChar("a"));
    REQUIRE(reader.getUtf8Char() == UniChar("m"));
    REQUIRE(reader.getUtf8Char() == UniChar("e"));
    REQUIRE(reader.getUtf8Char() == UniChar("n"));
    REQUIRE(reader.getUtf8Char() == UniChar("t"));
    REQUIRE(reader.getUtf8Char() == 0);
    return true;
}

TEST_CASE("string_view_utf8::Filament test", "[string_view_utf8]") {
    static const uint8_t utf8str[] = "Filament";
    string_view_utf8 sf = string_view_utf8::MakeRAM(utf8str);
    REQUIRE(Filament(sf));
}

TEST_CASE("string_view_utf8::Compute num of chars", "[string_view_utf8]") {
    {
        static const uint8_t utf8str[] = "";
        string_view_utf8 sf = string_view_utf8::MakeRAM(utf8str);
        REQUIRE(sf.computeNumUtf8Chars() == 0);
    }
    {
        static const uint8_t utf8str[] = "1";
        string_view_utf8 sf = string_view_utf8::MakeRAM(utf8str);
        REQUIRE(sf.computeNumUtf8Chars() == 1);
    }
    {
        static const uint8_t utf8str[] = "12";
        string_view_utf8 sf = string_view_utf8::MakeRAM(utf8str);
        REQUIRE(sf.computeNumUtf8Chars() == 2);
    }
    {
        static const uint8_t utf8str[] = "ěščř";
        string_view_utf8 sf = string_view_utf8::MakeRAM(utf8str);
        REQUIRE(sf.computeNumUtf8Chars() == 4);
    }
}

TEST_CASE("string_view_utf8::Copy to RAM", "[string_view_utf8]") {
    using Catch::Matchers::Equals;
    static const char fmt2Translate[] = "Nozzle: %.1f\177C";
    char fmt[21];

    const auto orig = string_view_utf8::MakeRAM((const uint8_t *)fmt2Translate);
    string_view_utf8 sf = orig;
    sf.copyToRAM(fmt, 1);
    REQUIRE_THAT(fmt, Equals("N"));

    sf = orig;
    sf.copyToRAM(fmt, 2);
    REQUIRE_THAT(fmt, Equals("No"));

    sf = orig;
    sf.copyToRAM(fmt, 4);
    REQUIRE_THAT(fmt, Equals("Nozz"));

    sf = orig;
    sf.copyToRAM(fmt, sizeof(fmt));
    REQUIRE_THAT(fmt, Equals(fmt2Translate));
}
