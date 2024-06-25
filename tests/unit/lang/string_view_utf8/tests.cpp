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

bool TheYellowHorse(const string_view_utf8 &sf) {
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

bool Cooldown(const string_view_utf8 &sf) {
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
    const string_view_utf8 &sf = string_view_utf8::MakeRAM(utf8str);
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
    size_t copied_bytes = sf.copyToRAM(fmt, 1);
    REQUIRE_THAT(fmt, Equals(""));
    REQUIRE(copied_bytes == 0);

    sf = orig;
    copied_bytes = sf.copyToRAM(fmt, 2);
    REQUIRE_THAT(fmt, Equals("N"));
    REQUIRE(copied_bytes == 1);

    sf = orig;
    copied_bytes = sf.copyToRAM(fmt, 4);
    REQUIRE_THAT(fmt, Equals("Noz"));
    REQUIRE(copied_bytes == 3);

    sf = orig;
    copied_bytes = sf.copyToRAM(fmt, sizeof(fmt));
    REQUIRE_THAT(fmt, Equals(fmt2Translate));
    REQUIRE(copied_bytes == sizeof(fmt2Translate) - 1);
}

TEST_CASE("string_view_utf8::CopyToRAM dst buffer too small + multibyte chars", "[string_view_utf8]") {
    using Catch::Matchers::Equals;
    char dst[44] = {};
    static const char src[] = "%d インプットシェーパーキャリブレーション";
    static const char ref[] = "%d インプットシェーパーキャリ";

    static auto orig = string_view_utf8::MakeRAM((const uint8_t *)src);
    size_t copied_bytes = orig.copyToRAM(dst, sizeof(dst));
    REQUIRE_THAT(dst, Equals(ref));
    REQUIRE(copied_bytes == sizeof(ref) - 1); // -1 because we don't count copying null at the end

    copied_bytes = orig.copyBytesToRAM(dst, sizeof(dst));
    REQUIRE(copied_bytes == sizeof(dst) - 1);
}

TEST_CASE("string_view_utf8::string_build", "[string_view_utf8]") {
    using Catch::Matchers::Equals;
    char compare_buff[60] = { 0 };

    static constexpr char fmt_test[] = "%s%d%s%.2f%%%ld%s";
    static constexpr char ref_test[] = "MK4236.2.010.50%1000000money";
    StringViewUtf8Parameters<33> params_test;
    string_view_utf8 str = string_view_utf8::MakeRAM(fmt_test).formatted(params_test, "MK4", 23, "6.2.0", 10.5, 1000000, "money");
    size_t copied_bytes = str.copyToRAM(compare_buff, sizeof(compare_buff));
    REQUIRE_THAT(compare_buff, Equals(ref_test));
    REQUIRE(copied_bytes == strlen(ref_test));

    static constexpr char fmt_one_char[] = "%d";
    static constexpr char ref_one_char[] = "3";
    StringViewUtf8Parameters<2> params_one_char;
    str = string_view_utf8::MakeRAM(fmt_one_char).formatted(params_one_char, 3);
    copied_bytes = str.copyToRAM(compare_buff, sizeof(compare_buff));
    REQUIRE_THAT(compare_buff, Equals(ref_one_char));
    REQUIRE(copied_bytes == strlen(ref_one_char));

    static const char fmt_truncate[] = "%d インプットシェーパーキャリブレーション";
    static const char ref_truncate[] = "9 インプットシェーパーキャリ";
    char buffer_too_small[44] = {};
    StringViewUtf8Parameters<2> params_truncate;
    str = string_view_utf8::MakeRAM(fmt_truncate).formatted(params_truncate, 9);
    copied_bytes = str.copyToRAM(buffer_too_small, sizeof(buffer_too_small));
    REQUIRE_THAT(buffer_too_small, Equals(ref_truncate));
    REQUIRE(copied_bytes == strlen(ref_truncate));

    static const char fmt_escape[] = "%%%s%%%d%%%.1f%%%%";
    static const char ref_escape[] = "%heh%0%0.0%%";
    StringViewUtf8Parameters<10> params_escape;
    str = string_view_utf8::MakeRAM(fmt_escape).formatted(params_escape, "heh", 0, 0.0f);
    copied_bytes = str.copyToRAM(compare_buff, sizeof(compare_buff));
    REQUIRE_THAT(compare_buff, Equals(ref_escape));
    REQUIRE(copied_bytes == strlen(ref_escape));

    /*
    static const char fmt_empty_str[] = "%s%s%s";
    StringViewUtf8Parameters<1> params_empty_str;
    str = string_view_utf8::MakeRAM(fmt_empty_str).formatted(params_empty_str, "", "", "");
    copied_bytes = str.copyToRAM(compare_buff, sizeof(compare_buff));
    REQUIRE_THAT(compare_buff, Equals(""));
    REQUIRE(copied_bytes == 0);
    */

    static const char pra[] = "pra";
    static const char fmt_text_in_between[] = "%s%s%sbabicka rekla \"%s%s%sdedo, ty si ale %sse\"";
    static const char ref_text_in_between[] = "prapraprababicka rekla \"praprapradedo, ty si ale prase\"";
    StringViewUtf8Parameters<28> params_text_in_between;
    str = string_view_utf8::MakeRAM(fmt_text_in_between).formatted(params_text_in_between, pra, pra, pra, pra, pra, pra, pra);
    copied_bytes = str.copyToRAM(compare_buff, sizeof(compare_buff));
    REQUIRE_THAT(compare_buff, Equals(ref_text_in_between));
    REQUIRE(copied_bytes == strlen(ref_text_in_between));
}
